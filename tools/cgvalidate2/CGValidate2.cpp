/**
 * File: CGValidate2.cpp
 * License: Part of the MetaCG project. Licensed under BSD 3 clause license. See LICENSE.txt file at
 * https://github.com/tudasc/metacg/LICENSE.txt
 */

#include "metacg/config.h"

#include "metacg/LoggerUtil.h"
#include "metacg/MCGManager.h"
#include "metacg/io/MCGReader.h"
#include "metacg/io/MCGWriter.h"

// This may appear to be unused, but the variables declared here have side effects on the graph lib
#include "metacg/metadata/BuiltinMD.h"
#include "metacg/metadata/OverrideMD.h"

#include <Cube.h>
#include <cxxopts.hpp>

#include <fstream>
#include <map>
#include <set>
#include <string>
#include <unordered_set>

using namespace metacg;

static double getVisits(cube::Cube& cube, const cube::Cnode* cn) {
  auto* metric = cube.get_met("visits");
  double total = 0.0;
  for (auto* thread : cube.get_thrdv()) {
    total += cube.get_sev(metric, cn, thread);
  }
  return total;
}

// Recursively collects NodeIds of all base functions that nodeId overrides (follows OverrideMD::overrides chain).
static std::unordered_set<NodeId> getAllBaseOverrides(NodeId nodeId, const Callgraph& cg) {
  std::unordered_set<NodeId> result;
  const auto* node = cg.getNode(nodeId);
  if (!node || !node->has<OverrideMD>()) {
    return result;
  }
  for (auto baseId : node->get<OverrideMD>()->overrides) {
    if (result.insert(baseId).second) {
      for (auto sub : getAllBaseOverrides(baseId, cg)) {
        result.insert(sub);
      }
    }
  }
  return result;
}

int main(int argc, char** argv) {
  auto console = MCGLogger::instance().getConsole();
  auto errConsole = MCGLogger::instance().getErrConsole();

  cxxopts::Options options(
      "cgvalidate2",
      "Validation of MCG files using cubex files.\n"
      "Note: -u reports normalized call counts from Cube but does not write them to the output MCG.");
  options.add_options()("i,mcg", "MCG file name", cxxopts::value<std::string>())("c,cubex", "cubex file name",
                                                                                 cxxopts::value<std::string>())(
      "b,useNoBodyDetection", "Tolerate missing edges when caller or callee lacks a body definition",
      cxxopts::value<bool>()->default_value("false"))("p,patch", "Patch MCG using the cubex file",
                                                      cxxopts::value<bool>()->default_value("false"))(
      "o,output", "Output file for patched MCG (default: <mcg>.patched)",
      cxxopts::value<std::string>()->default_value(""))("n,noNewNodes", "Disable insertion of new nodes when patching",
                                                        cxxopts::value<bool>()->default_value("false"))(
      "u,useCubeCallCount", "Log normalized call counts from Cube alongside mismatches (not written to output MCG)",
      cxxopts::value<bool>()->default_value("false"))("h,help", "Print help");

  const auto result = options.parse(argc, argv);
  if (result.count("help")) {
    std::cout << options.help() << "\n";
    return 0;
  }

  const std::string mcgFile = result["mcg"].as<std::string>();
  const std::string cubexFile = result["cubex"].as<std::string>();
  const bool patch = result["patch"].as<bool>();
  std::string outputFile = result["output"].as<std::string>();
  if (patch && outputFile.empty()) {
    outputFile = mcgFile + ".patched";
  }
  const bool useNoBodyDetection = result["useNoBodyDetection"].as<bool>();
  const bool insertNewNodes = !(result["noNewNodes"].as<bool>());
  const bool useCubeCallCounts = result["useCubeCallCount"].as<bool>();

  console->info("Running metacg::CGValidate2 (version {}.{})\nGit revision: {}", MetaCG_VERSION_MAJOR,
                MetaCG_VERSION_MINOR, MetaCG_GIT_SHA);
  console->info("{}", mcgFile);
  console->info("{}", cubexFile);

  // Load MCG
  io::FileSource src(mcgFile);
  int mcgVersion = 4;
  try {
    const std::string versionStr = src.getFormatVersion();
    if (versionStr == "unknown") {
      errConsole->error("[Error] Could not determine MCG format version for {}", mcgFile);
      return 2;
    }
    mcgVersion = std::stoi(versionStr);
  } catch (const std::exception& e) {
    errConsole->error("[Error] MCG file {} not readable: {}", mcgFile, e.what());
    return 2;
  }

  auto reader = io::createReader(src);
  if (!reader) {
    errConsole->error("[Error] Unsupported MCG format version {} for {}", mcgVersion, mcgFile);
    return 2;
  }

  auto& mcgManager = graph::MCGManager::get();
  mcgManager.resetManager();
  auto cgOwned = reader->read();
  Callgraph* cg = cgOwned.get();
  mcgManager.addToManagedGraphs(mcgFile, std::move(cgOwned), true);

  // Load Cube
  cube::Cube cube;
  try {
    cube.openCubeReport(cubexFile);
  } catch (const std::exception& e) {
    errConsole->error("[Error] cube file {} not readable: {}", cubexFile, e.what());
    return 3;
  }

  // Build per-edge and total call counts from Cube (used for -u reporting)
  std::map<std::string, double> totalCallCounts;
  std::map<std::string, std::map<std::string, double>> callCounts;
  const auto& cnodes = cube.get_cnodev();
  for (const auto* cnode : cnodes) {
    const std::string calledName = cnode->get_callee()->get_mangled_name();
    const double visits = getVisits(cube, cnode);
    totalCallCounts[calledName] += visits;
    if (const auto* caller = cnode->get_caller()) {
      callCounts[caller->get_mangled_name()][calledName] += visits;
    }
  }

  // Helper: look up a node by name, inserting it if allowed and not present.
  // Returns nullptr if the node doesn't exist and insertion is disabled.
  const auto ensureNode = [&](const std::string& name) -> CgNode* {
    if (!cg->hasNode(name)) {
      if (insertNewNodes) {
        auto& n = cg->insert(name);
        console->warn("[Warning] Inserted previously undeclared node {}", name);
        return &n;
      } else {
        console->warn("[Warning] Not inserted undeclared node {}", name);
        return nullptr;
      }
    }
    return cg->getFirstNode(name);
  };

  // Validate Cube edges against the MCG
  bool verified = true;
  std::set<std::pair<std::string, std::string>> edgesChecked;

  for (const auto* cnode : cnodes) {
    if (!cnode->get_parent()) {
      continue;
    }
    const std::string nodeName = cnode->get_callee()->get_mangled_name();
    if (nodeName == "main") {
      continue;
    }
    const std::string parentName = cnode->get_parent()->get_callee()->get_mangled_name();

    edgesChecked.emplace(parentName, nodeName);

    CgNode* parentNode = ensureNode(parentName);
    if (!parentNode) {
      continue;
    }
    CgNode* nodePtr = ensureNode(nodeName);
    if (!nodePtr) {
      continue;
    }

    bool edgeFound = cg->existsAnyEdge(parentName, nodeName);

    // Check polymorphism: Cube may report a call to a base-class method while the IPCG
    // records the edge to the overriding derived-class method.
    bool overrideEdgeFound = false;
    for (auto baseId : getAllBaseOverrides(nodePtr->getId(), *cg)) {
      const auto* baseNode = cg->getNode(baseId);
      if (!baseNode) {
        continue;
      }
      if (cg->existsAnyEdge(parentName, baseNode->getFunctionName())) {
        overrideEdgeFound = true;
        break;
      }
    }

    if (useNoBodyDetection) {
      const bool pHasBody = parentNode->getHasBody();
      const bool cHasBody = nodePtr->getHasBody();
      if (!pHasBody) {
        errConsole->warn("[Warning] No CGCollector data for {} in MCG.", parentName);
      }
      if (!cHasBody) {
        errConsole->warn("[Warning] No CGCollector data for {} in MCG.", nodeName);
      }
      // If either side lacks a body we cannot expect the static edge to be present.
      edgeFound = edgeFound || !pHasBody || !cHasBody;
    }

    if (!edgeFound && !overrideEdgeFound) {
      errConsole->info("[Error] {} does not contain parent {}", nodeName, parentName);
      errConsole->info("[Error] {} does not contain callee {}", parentName, nodeName);
      verified = false;
      if (patch) {
        ensureNode(parentName);
        ensureNode(nodeName);
        cg->addEdge(parentName, nodeName);
        console->info("[Info] patched in edge {} -> {} in {}", parentName, nodeName, outputFile);
      }
    } else if (useCubeCallCounts) {
      const double total = totalCallCounts.count(parentName) ? totalCallCounts.at(parentName) : 0.0;
      const double count = (callCounts.count(parentName) && callCounts.at(parentName).count(nodeName))
                               ? callCounts.at(parentName).at(nodeName)
                               : 0.0;
      console->info("[Info] Cube call count for {} -> {}: {:.6f}", parentName, nodeName,
                    total > 0.0 ? count / total : 0.0);
    }
  }

  console->info("[Info] Checked {} edges.", edgesChecked.size());

  if (!verified) {
    if (patch) {
      auto writer = io::createWriter(mcgVersion);
      if (!writer) {
        errConsole->error("Unable to create writer for format version {}", mcgVersion);
        return 1;
      }
      io::JsonSink jsonSink;
      writer->writeActiveGraph(jsonSink);
      std::ofstream os(outputFile);
      os << jsonSink.getJson() << "\n";
    }
    return 1;
  }

  console->info("callgraph does match cube file");
  return 0;
}

#include "config.h"

#include "JSONManager.h"

#include "cxxopts.hpp"

#include <Cube.h>

#include <iostream>
#include <set>
#include <string>

#ifndef LOGLEVEL
#define LOGLEVEL 0
#endif

// Parse options
void handleOptions(int argc, char **argv, std::string &ipcg, std::string &cubex, bool &patch, std::string &output,
                   bool &useNoBodyDetection, bool &insertNewNodes) {
  // clang-format off
  cxxopts::Options options("cgvalidate", "Validation of ipcg files using cubex files");
  options.add_options()("i,ipcg", "ipcg file name", cxxopts::value<std::string>())(
      "c,cubex", "cubex file name", cxxopts::value<std::string>())("b,useNoBodyDetection",
                                                                   "handling of existing function definitions",
                                                                   cxxopts::value<bool>()->default_value("false"))(
      "p,patch", "patch ipcg using the cubex file", cxxopts::value<bool>()->default_value("false"))(
      "o,output", "output file for patched ipcg", cxxopts::value<std::string>()->default_value(""))(
      "n,noNewNodes", "disable adding of new nodes by patch", cxxopts::value<bool>()->default_value("false"))(
      "h,help", "Print help");
  // clang-format on
  cxxopts::ParseResult result = options.parse(argc, argv);

  if (result.count("help")) {
    std::cout << options.help({""}) << std::endl;
    exit(0);
  }

  ipcg = result["ipcg"].as<std::string>();
  cubex = result["cubex"].as<std::string>();
  patch = result["patch"].as<bool>();
  output = result["output"].as<std::string>();
  if (patch && output.compare("") == 0) {
    output = ipcg + ".patched";
  }
  useNoBodyDetection = result["useNoBodyDetection"].as<bool>();
  insertNewNodes = !(result["noNewNodes"].as<bool>());

  std::cout << ipcg << std::endl;
  std::cout << cubex << std::endl;
}

void readCube(const std::string &filename, cube::Cube &cube) { cube.openCubeReport(filename); }

bool isMain(const std::string &mangledName) { return mangledName == "main"; }

bool getOrInsert(nlohmann::json &callgraph, const std::string &nodeName, const bool insertNewNodes) {
  if (callgraph.contains(nodeName)) {
    return true;
  } else {
    if (insertNewNodes) {
      insertDefaultNode(callgraph, nodeName);
      std::cout << "[Warning] Inserted previously undeclared node " << nodeName << std::endl;
      return true;
    } else {
      std::cout << "[Warning] Not inserted undeclared node " << nodeName << std::endl;
      return false;
    }
  }
}

void patchCallgraph(nlohmann::json &callgraph, const std::string &nodeName, const std::string &valueName,
                    const std::string &mode, const std::string &outputFileName, const bool insertNewNodes) {
  auto nExists = getOrInsert(callgraph, nodeName, insertNewNodes);
  auto valueExists = getOrInsert(callgraph, valueName, insertNewNodes);

  if (nExists && valueExists) {
    callgraph[nodeName][mode].push_back(valueName);
    std::cout << "patched in " << outputFileName << std::endl;
  }
}

std::set<std::pair<std::string, std::string>> edgesChecked;

int main(int argc, char **argv) {
  std::string ipcg;
  std::string cubex;
  bool patch;
  std::string output;
  /**
   * If we don't have a function's definition, we cannot find any edges anyway.
   */
  bool useNoBodyDetection;
  bool insertNewNodes;

  handleOptions(argc, argv, ipcg, cubex, patch, output, useNoBodyDetection, insertNewNodes);

  std::cout << "Running metacg::CGValidate (version " << CGCollector_VERSION_MAJOR << '.' << CGCollector_VERSION_MINOR
            << ")\nGit revision: " << MetaCG_GIT_SHA << std::endl;

  nlohmann::json callgraph;
  try {
    readIPCG(ipcg, callgraph);
  } catch (std::exception e) {
    std::cerr << "[Error] ipcg file " << ipcg << " not readable" << std::endl;
    return 2;
  }

  cube::Cube cube;
  try {
    readCube(cubex, cube);
  } catch (std::exception e) {
    std::cerr << "[Error] cube file " << cubex << " not readable" << std::endl;
    return 3;
  }

  // iterate over cube to check if edges are in callgraph
  bool verified = true;
  const auto &cnodes = cube.get_cnodev();
  for (const auto cnode : cnodes) {
    // get cube elements
    // continue if we are in root element or main
    if (!cnode->get_parent()) {
      continue;
    }
    std::string nodeName = cnode->get_callee()->get_mangled_name();
    if (isMain(nodeName)) {
      continue;
    }
    std::string parentName = cnode->get_parent()->get_callee()->get_mangled_name();

    if (LOGLEVEL > 0) {
      std::cout << "[INFO] edge reached: " << parentName << " --> " << nodeName << std::endl;
    }
    edgesChecked.insert(std::make_pair(parentName, nodeName));

    // get callgraph elements
    if (!getOrInsert(callgraph, parentName, insertNewNodes)) {
      continue;
    }
    auto parent = callgraph[parentName];
    auto callees = parent["callees"];
    if (!getOrInsert(callgraph, nodeName, insertNewNodes)) {
      continue;
    }
    auto node = callgraph[nodeName];
    auto parents = node["parents"];

    // check if parent contains callee and callee contains parent
    auto calleeFound =
        (std::find(callees.begin(), callees.end(), nodeName) != callees.end());  // doesInclude(callees, nodeName);
    auto parentFound =
        (std::find(parents.begin(), parents.end(), parentName) != parents.end());  // doesInclude(parents, parentName);
    // check polymorphism (currently only first hierarchy level)
    bool overriddenFunctionParentFound = false;
    bool overriddenFunctionCalleeFound = false;
    const auto &overriddenFunctions = node["overriddenFunctions"];
    for (const std::string overriddenFunctionName : overriddenFunctions) {
      if (!getOrInsert(callgraph, overriddenFunctionName, insertNewNodes)) {
        continue;
      }
      const auto &overriddenFunction = callgraph[overriddenFunctionName];
      const auto &parents = overriddenFunction["parents"];
      overriddenFunctionParentFound = (std::find(parents.begin(), parents.end(), parentName) != parents.end());
      if (overriddenFunctionParentFound) {
        break;
      }
    }
    for (const std::string overriddenFunctionName : overriddenFunctions) {
      overriddenFunctionCalleeFound =
          (std::find(callees.begin(), callees.end(), overriddenFunctionName) != callees.end());
      if (overriddenFunctionCalleeFound) {
        break;
      }
    }

    if (useNoBodyDetection) {
      bool pHasBody{true};
      bool cHasBody{true};
      if (parent["hasBody"].is_null()) {
        std::cerr << "[Warning] No CGCollector data for " << parentName << " in IPCG." << std::endl;
      } else {
        pHasBody = parent["hasBody"].get<bool>();
      }
      if (node["hasBody"].is_null()) {
        std::cerr << "[Warning] No CGCollector data for " << nodeName << " in IPCG." << std::endl;
      } else {
        cHasBody = node["hasBody"].get<bool>();
      }
      calleeFound = calleeFound || !pHasBody;  // if no body av, how should we know?
      parentFound = parentFound || !cHasBody;  // if no body av, how should we know?
    }

    // reporting
    if (!parentFound && !overriddenFunctionParentFound) {
      std::cout << "[Error] " << nodeName << " does not contain parent " << parentName << std::endl;
      verified = false;
      if (patch) {
        patchCallgraph(callgraph, nodeName, parentName, "parents", output, insertNewNodes);
      }
    }
    if (!calleeFound && !overriddenFunctionCalleeFound) {
      std::cout << "[Error] " << parentName << " does not contain callee " << nodeName << std::endl;
      verified = false;
      if (patch) {
        patchCallgraph(callgraph, parentName, nodeName, "callees", output, insertNewNodes);
      }
    }
  }

  /*
   * finalize
   */
  std::cout << "[Info] Checked " << edgesChecked.size() << " edges." << std::endl;
  if (!verified) {
    if (patch) {
      writeIPCG(output, callgraph);
    }
    return 1;
  } else {
    std::cout << "callgraph does match cube file" << std::endl;
    return 0;
  }
}

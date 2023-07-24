#include "config.h"

#include "JSONManager.h"
#include "MetaInformation.h"

#include "cxxopts.hpp"

#include <Cube.h>

#include <iostream>
#include <set>
#include <string>

#ifndef LOGLEVEL
#define LOGLEVEL 0
#endif

static bool hasCallCount = false;

std::vector<std::string> newNodes;
std::map<std::string, double> totalCallCounts;
std::map<std::string, std::map<std::string, double>> callCounts;

// Parse options
void handleOptions(int argc, char **argv, std::string &ipcg, std::string &cubex, bool &patch, std::string &output,
                   bool &useNoBodyDetection, bool &insertNewNodes, bool &useCubeForCallCounts) {
  // clang-format off
  cxxopts::Options options("cgvalidate", "Validation of ipcg files using cubex files");
  options.add_options()("i,ipcg", "ipcg file name", cxxopts::value<std::string>())(
      "c,cubex", "cubex file name", cxxopts::value<std::string>())("b,useNoBodyDetection",
                                                                   "handling of existing function definitions",
                                                                   cxxopts::value<bool>()->default_value("false"))(
      "p,patch", "patch ipcg using the cubex file", cxxopts::value<bool>()->default_value("false"))(
      "o,output", "output file for patched ipcg", cxxopts::value<std::string>()->default_value(""))(
      "n,noNewNodes", "disable adding of new nodes by patch", cxxopts::value<bool>()->default_value("false"))(
      "u,useCubeCallCount" , "overwrite the call counts in the ipcg with the ones obtained from the cubex file",
            cxxopts::value<bool>()->default_value("false"))(
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
  useCubeForCallCounts = result["useCubeCallCount"].as<bool>();

  std::cout << ipcg << std::endl;
  std::cout << cubex << std::endl;
}

void readCube(const std::string &filename, cube::Cube &cube) { cube.openCubeReport(filename); }

bool isMain(const std::string &mangledName) { return mangledName == "main"; }

bool getOrInsert(nlohmann::json &callgraph, const std::string &nodeName, const bool insertNewNodes, const int version) {
  if (callgraph.contains(nodeName)) {
    return true;
  } else {
    if (insertNewNodes) {
      insertDefaultNode(callgraph, nodeName, version, true);
      newNodes.push_back(nodeName);
      if (version == 2) {
        std::map<std::string, double> callMap;
        CodeRegionsType codeRegions;
        CalledFunctionType calledFunctions;
        callgraph[nodeName]["meta"]["estimateCallCount"]["calls"] = calledFunctions;
        callgraph[nodeName]["meta"]["estimateCallCount"]["codeRegions"] = codeRegions;
      }
      std::cout << "[Warning] Inserted previously undeclared node " << nodeName << std::endl;
      return true;
    } else {
      std::cout << "[Warning] Not inserted undeclared node " << nodeName << std::endl;
      return false;
    }
  }
}

/** Function to patch in new call into the callgraph
 * If the option "useCubeCallCount" is given or if the call did not exist in the callgraph, perfect call counts based on
 * the cube measurements are patched in
 *
 * @param callgraph Reference to the callgraph json representation
 * @param nodeName Name of the function where the metadata are updated
 * @param valueName Name of the function that should be added to the metadata
 * @param mode Field where the metadata should be updated. "callees" or "callers" or "parents"
 * @param outputFileName The name of the IPCG file that is written to, only used for logging purposes
 * @param insertNewNodes Should nodes be inserted if they do not exist? Otherwise this function silently skips patching
 * in a call if "node" or "value" do not exist.
 * @param version IPCG file format version
 */
void patchCallgraph(nlohmann::json &callgraph, const std::string &nodeName, const std::string &valueName,
                    const std::string &mode, const std::string &outputFileName, const bool insertNewNodes,
                    const int version) {
  auto nodeExists = getOrInsert(callgraph, nodeName, insertNewNodes, version);
  auto valueExists = getOrInsert(callgraph, valueName, insertNewNodes, version);

  if (nodeExists && valueExists) {
    // Temporary variable because getting a mutable reference does not work for some reason
    auto t = callgraph[nodeName][mode].get<std::set<std::string>>();
    if (t.find(valueName) == t.end()) {
      t.insert(valueName);
      std::cout << "patched in " << outputFileName << std::endl;
      callgraph[nodeName][mode] = t;
    }
    if (mode == "callees" && callgraph[nodeName].contains("meta") &&
        callgraph[nodeName]["meta"].contains("estimateCallCount")) {
      hasCallCount = true;
      CalledFunctionType calledFunctions =
          callgraph[nodeName]["meta"]["estimateCallCount"]["calls"].get<CalledFunctionType>();
      const double callCount = callCounts[nodeName][valueName] / totalCallCounts[nodeName];
      assert(callCount != 0);
      {
        auto functionIter = calledFunctions.find(valueName);
        std::set<std::pair<double, std::string>> callInfoReplacement;
        callInfoReplacement.emplace(callCount, "");
        if (functionIter != calledFunctions.end()) {
          // Patch in the perfect value and remove all others
          functionIter->second = callInfoReplacement;
        } else {
          calledFunctions.emplace(valueName, callInfoReplacement);
        }
      }
      callgraph[nodeName]["meta"]["estimateCallCount"]["calls"] = calledFunctions;
    }
  }
}

std::set<std::pair<std::string, std::string>> edgesChecked;

// Ripped from pgis
const auto cMetric = [](std::string &&name, auto &&cube, auto cn) {
  if constexpr (std::is_pointer_v<decltype(cn)>) {
    const auto met = cube.get_met(name.c_str());
    typedef decltype(cube.get_sev(met, cn, cube.get_thrdv().at(0))) RetType;
    RetType metric{};
    for (auto t : cube.get_thrdv()) {
      metric += cube.get_sev(met, cn, t);
    }
    return metric;
  } else {
    assert(false);
  }
};
const auto getVisits = [](auto &&cube, auto cn) { return cMetric(std::string("visits"), cube, cn); };

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
  bool useCubeCallCounts;

  handleOptions(argc, argv, ipcg, cubex, patch, output, useNoBodyDetection, insertNewNodes, useCubeCallCounts);

  std::cout << "Running metacg::CGValidate (version " << CGCollector_VERSION_MAJOR << '.' << CGCollector_VERSION_MINOR
            << ")\nGit revision: " << MetaCG_GIT_SHA << std::endl;

  nlohmann::json callgraph;
  try {
    readIPCG(ipcg, callgraph);
  } catch (std::exception e) {
    std::cerr << "[Error] ipcg file " << ipcg << " not readable" << std::endl;
    return 2;
  }
  nlohmann::json cgback = callgraph;

  int version = 1;

  if (callgraph.contains("_CG")) {
    version = 2;
    callgraph = callgraph["_CG"];
  }
  const std::string parentKey = (version == 2) ? "callers" : "parents";

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
    const auto calledName = cnode->get_callee()->get_mangled_name();
    const auto visits = getVisits(cube, cnode);
    totalCallCounts[calledName] += visits;
    const auto caller = cnode->get_caller();
    if (caller) {
      const auto caller_name = cnode->get_caller()->get_mangled_name();
      assert(!caller_name.empty());
      callCounts[caller_name][calledName] += visits;
    }
  }

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
    if (!getOrInsert(callgraph, parentName, insertNewNodes, version)) {
      continue;
    }
    auto parent = callgraph[parentName];
    auto callees = parent["callees"];
    if (!getOrInsert(callgraph, nodeName, insertNewNodes, version)) {
      continue;
    }
    auto node = callgraph[nodeName];
    auto parents = node[parentKey];

    // check if parent contains callee and callee contains parent
    auto calleeFound =
        (std::find(callees.begin(), callees.end(), nodeName) != callees.end());    // doesInclude(callees, nodeName);
    auto parentFound =
        (std::find(parents.begin(), parents.end(), parentName) != parents.end());  // doesInclude(parents, parentName);
    // check polymorphism (currently only first hierarchy level)
    bool overriddenFunctionParentFound = false;
    bool overriddenFunctionCalleeFound = false;
    const auto &overriddenFunctions = node["overriddenFunctions"];
    for (const std::string overriddenFunctionName : overriddenFunctions) {
      if (!getOrInsert(callgraph, overriddenFunctionName, insertNewNodes, version)) {
        continue;
      }
      const auto &overriddenFunction = callgraph[overriddenFunctionName];
      const auto &parents = overriddenFunction[parentKey];
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
        patchCallgraph(callgraph, nodeName, parentName, parentKey, output, insertNewNodes, version);
      }
    }
    if ((!calleeFound && !overriddenFunctionCalleeFound) || useCubeCallCounts) {
      if (!calleeFound && !overriddenFunctionCalleeFound) {
        std::cout << "[Error] " << parentName << " does not contain callee " << nodeName << std::endl;
      } else {
        std::cout << "[Info] patching in cube call counts for " << parentName << std::endl;
      }
      verified = false;
      if (patch) {
        patchCallgraph(callgraph, parentName, nodeName, "callees", output, insertNewNodes, version);
      }
    }
  }
  if (useCubeCallCounts) {
    for (auto &[key, value] : callgraph.items()) {
      auto nExists = getOrInsert(callgraph, key, insertNewNodes, version);
      const auto nodeName = key;
      assert(nExists);

      if (callgraph[nodeName].contains("meta") && callgraph[nodeName]["meta"].contains("estimateCallCount")) {
        CalledFunctionType calledFunctions =
            callgraph[nodeName]["meta"]["estimateCallCount"]["calls"].get<CalledFunctionType>();
        for (const auto &calledFunction : calledFunctions) {
          const auto calledFunctionName = calledFunction.first;
          if (edgesChecked.find({nodeName, calledFunctionName}) == edgesChecked.end()) {
            auto functionIter = calledFunctions.find(calledFunctionName);
            // Zero it
            std::set<std::pair<double, std::string>> callInfoReplacement;
            callInfoReplacement.emplace(0, "");
            functionIter->second = callInfoReplacement;
          }
        }
        callgraph[nodeName]["meta"]["estimateCallCount"]["calls"] = calledFunctions;
      }
    }
  }

  /*
   * finalize
   */
  std::cout << "[Info] Checked " << edgesChecked.size() << " edges." << std::endl;
  if (!verified) {
    if (patch) {
      if (version == 2) {
        cgback["_CG"] = callgraph;
        callgraph = cgback;
      }
      if (hasCallCount) {
        for (const auto &node : newNodes) {
          if (!callgraph["_CG"][node]["meta"].contains("estimateCallCount")) {
            std::map<std::string, double> callMap;

            callgraph["_CG"][node]["meta"]["estimateCallCount"] = callMap;
          }
        }
      }
      writeIPCG(output, callgraph);
    }
    return 1;
  } else {
    std::cout << "callgraph does match cube file" << std::endl;
    return 0;
  }
}

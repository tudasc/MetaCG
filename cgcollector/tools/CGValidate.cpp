#include "config.h"

#include "nlohmann/json.hpp"

#include <cubelib/Cube.h>

#include <iostream>
#include <string>

#ifndef LOGLEVEL
#define LOGLEVEL 0
#endif

void readIPCG(char const *const filename, nlohmann::json &callgraph) {
  std::ifstream file(filename);
  file >> callgraph;
}

void readCube(char const *const filename, cube::Cube &cube) { cube.openCubeReport(filename); }

bool isMain(const std::string &name) { return (name.compare("main") == 0); }

int main(int argc, char **argv) {
  if (argc != 3) {
    std::cerr << "CGValidate usage:\nPlease pass the ipcg file as the first and the cubex file as the second parameter"
              << std::endl;
    return 1;
  }

  std::cout << "Running MetaCG::CGValidate (version " << CGCollector_VERSION_MAJOR << '.' << CGCollector_VERSION_MINOR
            << ')' << std::endl;

  /**
   * If we don't have a function's definition, we cannot find any edges anyway.
   * XXX This should be a feature w/ a CLI swtich
   */
  bool useNoBodyDetection{false};

  nlohmann::json callgraph;
  try {
    readIPCG(argv[1], callgraph);
  } catch (std::exception e) {
    std::cerr << "[Error] ipcg file " << argv[1] << " not readable" << std::endl;
    return 2;
  }

  cube::Cube cube;
  try {
    readCube(argv[2], cube);
  } catch (std::exception e) {
    std::cerr << "[Error] cube file " << argv[2] << " not readable" << std::endl;
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

    // get callgraph elements
    auto parent = callgraph[parentName];
    auto callees = parent["callees"];
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
      const auto &overriddenFunction = callgraph[overriddenFunctionName];
      const auto &parents = overriddenFunction["parents"];
      overriddenFunctionParentFound = (std::find(parents.begin(), parents.end(), parentName) != parents.end());
      if (overriddenFunctionParentFound)
        break;
    }
    for (const std::string overriddenFunctionName : overriddenFunctions) {
      overriddenFunctionCalleeFound =
          (std::find(callees.begin(), callees.end(), overriddenFunctionName) != callees.end());
      if (overriddenFunctionCalleeFound)
        break;
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
    }
    if (!calleeFound && !overriddenFunctionCalleeFound) {
      std::cout << "[Error] " << parentName << " does not contain callee " << nodeName << std::endl;
      verified = false;
    }
  }

  if (!verified) {
    return 1;
  }

  std::cout << "callgraph does match cube file" << std::endl;
  return 0;
}

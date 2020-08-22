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
    std::cerr << "pleaes pass ipcg file as first and cubex file as second parameter" << std::endl;
    return 1;
  }

  nlohmann::json callgraph;
  try {
    readIPCG(argv[1], callgraph);
  } catch (std::exception e) {
    std::cerr << "ipcg not readable" << std::endl;
    return 1;
  }

  cube::Cube cube;
  try {
    readCube(argv[2], cube);
  } catch (std::exception e) {
    std::cerr << "cube not readable" << std::endl;
    return 1;
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

    // reporting
    if (!parentFound && !overriddenFunctionParentFound) {
      std::cout << "[ERROR] " << nodeName << " does not contain parent " << parentName << std::endl;
      verified = false;
    }
    if (!calleeFound && !overriddenFunctionCalleeFound) {
      std::cout << "[ERROR] " << parentName << " does not contain callee " << nodeName << std::endl;
      verified = false;
    }
  }

  if (!verified) {
    return 1;
  }

  std::cout << "callgraph does match cube file" << std::endl;
  return 0;
}

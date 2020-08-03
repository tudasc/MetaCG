#include "nlohmann/json.hpp"

#include <cubelib/Cube.h>

#include <iostream>
#include <string>

void readIPCG(char const *const filename, nlohmann::json &callgraph) {
  std::ifstream file(filename);
  file >> callgraph;
}

void readCube(char const *const filename, cube::Cube &cube) { cube.openCubeReport(filename); }

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
    // TODO check for main
    if (nodeName.compare("main") == 0) {
      continue;
    }
    std::string parentName = cnode->get_parent()->get_callee()->get_mangled_name();

    // get callgraph elements
    const auto &parent = callgraph[parentName];
    const auto &callees = parent["callees"];
    const auto &node = callgraph[nodeName];
    const auto &parents = node["parents"];

    // check if parent contains callee and callee contains parent
    bool calleeFound = (std::find(callees.begin(), callees.end(), nodeName) != callees.end());
    bool parentFound = (std::find(parents.begin(), parents.end(), parentName) != parents.end());

    // reporting
    if (!parentFound) {
      std::cout << "[ERROR] " << nodeName << " does not contain parent " << parentName << std::endl;
      verified = false;
    }
    if (!calleeFound) {
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

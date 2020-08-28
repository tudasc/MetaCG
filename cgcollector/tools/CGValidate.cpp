#include "config.h"

#include "cxxopts.hpp"
#include "nlohmann/json.hpp"

#include <cubelib/Cube.h>

#include <iostream>
#include <string>

#ifndef LOGLEVEL
#define LOGLEVEL 0
#endif

void readIPCG(std::string &filename, nlohmann::json &callgraph) {
  std::ifstream file(filename);
  file >> callgraph;
  file.close();
}

void writeIPCG(std::string &filename, nlohmann::json &callgraph) {
  std::ofstream file(filename);
  file << callgraph;
  file.close();
}

void readCube(std::string &filename, cube::Cube &cube) { cube.openCubeReport(filename); }

bool isMain(const std::string &name) { return (name.compare("main") == 0); }

void handleOptions(int argc, char **argv, std::string &ipcg, std::string &cubex, bool &fix, std::string &output,
                   bool &useNoBodyDetection) {
  cxxopts::Options options("cgvalidate", "Validation of ipcg files using cubex files");
  options.add_options()("i,ipcg", "ipcg file name", cxxopts::value<std::string>())(
      "c,cubex", "cubex file name", cxxopts::value<std::string>())("b,useNoBodyDetection",
                                                                   "handling of existing function definitions",
                                                                   cxxopts::value<bool>()->default_value("false"))(
      "f,fix", "fix ipcg using the cubex file", cxxopts::value<bool>()->default_value("false"))(
      "o,output", "output file for fixed ipcg", cxxopts::value<std::string>()->default_value(""))("h,help",
                                                                                                  "Print help");
  cxxopts::ParseResult result = options.parse(argc, argv);

  if (result.count("help")) {
    std::cout << options.help({""}) << std::endl;
    exit(0);
  }

  ipcg = result["ipcg"].as<std::string>();
  cubex = result["cubex"].as<std::string>();
  fix = result["fix"].as<bool>();
  output = result["output"].as<std::string>();
  if (fix && output.compare("") == 0) {
    output = ipcg + ".fix";
  }

  std::cout << ipcg << std::endl;
  std::cout << cubex << std::endl;
}

int main(int argc, char **argv) {
  std::string ipcg;
  std::string cubex;
  bool fix;
  std::string output;
  /**
   * If we don't have a function's definition, we cannot find any edges anyway.
   */
  bool useNoBodyDetection;

  handleOptions(argc, argv, ipcg, cubex, fix, output, useNoBodyDetection);

  std::cout << "Running MetaCG::CGValidate (version " << CGCollector_VERSION_MAJOR << '.' << CGCollector_VERSION_MINOR
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
      if (fix) {
        callgraph[nodeName]["parents"].push_back(parentName);
        std::cout << "fixed in " << output << std::endl;
      }
    }
    if (!calleeFound && !overriddenFunctionCalleeFound) {
      std::cout << "[Error] " << parentName << " does not contain callee " << nodeName << std::endl;
      verified = false;
      if (fix) {
        callgraph[parentName]["callees"].push_back(nodeName);
        std::cout << "fixed in " << output << std::endl;
      }
    }
  }

  if (fix) {
    writeIPCG(output, callgraph);
  }

  if (!verified) {
    return 1;
  }

  std::cout << "callgraph does match cube file" << std::endl;
  return 0;
}

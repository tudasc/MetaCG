/**
 * File: VersionThreeMCGReader.cpp
 * License: Part of the MetaCG project. Licensed under BSD 3 clause license. See LICENSE.txt file at
 * https://github.com/tudasc/metacg/LICENSE.txt
 */

#include "io/VersionThreeMCGReader.h"
#include "MCGBaseInfo.h"
#include "Timing.h"
#include "Util.h"
#include <iostream>
void metacg::io::VersionThreeMetaCGReader::read(metacg::graph::MCGManager &cgManager) {
  metacg::RuntimeTimer rtt("VersionThreeMetaCGReader::read");
  MCGFileFormatInfo ffInfo{3, 0};
  auto console = metacg::MCGLogger::instance().getConsole();
  auto errConsole = metacg::MCGLogger::instance().getErrConsole();

  auto j = source.get();
  auto mcgInfo = j[ffInfo.metaInfoFieldName];
  if (mcgInfo.is_null()) {
    errConsole->error("Could not read version info from metacg file.");
    throw std::runtime_error("Could not read version info from metacg file");
  }
  /// XXX How to make that we can use the MCGGeneratorVersionInfo to access the identifiers
  auto mcgVersion = mcgInfo["version"].get<std::string>();
  auto generatorName = mcgInfo["generator"]["name"].get<std::string>();
  auto generatorVersion = mcgInfo["generator"]["version"].get<std::string>();
  MCGGeneratorVersionInfo genVersionInfo{generatorName, metacg::util::getMajorVersionFromString(generatorVersion),
                                         metacg::util::getMinorVersionFromString(generatorVersion), ""};
  console->info("The metacg (version {}) file was generated with {} (version: {})", mcgVersion, generatorName,
                generatorVersion);
  if (mcgVersion.at(0) != '3') {
    console->info("This reader can only read metacg (version 2) files");
    throw std::runtime_error("Trying to read incompatible file with MCGReader3");
  }
  MCGFileInfo fileInfo{ffInfo, genVersionInfo};
  auto &jsonCG = j[ffInfo.cgFieldName];
  if (jsonCG.is_null()) {
    errConsole->error("The call graph in the metacg file was null.");
    throw std::runtime_error("CG in metacg file was null.");
  }

  if (!jsonCG.contains("nodes") || !jsonCG.contains("edges")) {
    errConsole->error("Could not find nodes and edges in file.");
    throw std::runtime_error("Node/Edge container not found");
  }
  if (isV3DebugFormat(jsonCG)) {
    convertFromDebug(jsonCG);
  }
  cgManager.addToManagedGraphs("newGraph", std::make_unique<Callgraph>(jsonCG));
}

void metacg::io::VersionThreeMetaCGReader::convertFromDebug(json &j) {
  // replace the identifier strings with their hash
  for (auto &node : j["nodes"]) {
    std::string name=node.at(0);
    if(node.at(1).contains("origin")){
      name+=node.at(1).at("origin");
    }else{
      assert(false);
      name+="unknownOrigin";
    }
    node.at(0) = std::hash<std::string>()(name);
    node.at(1).erase("callers");
    node.at(1).erase("callees");
  }

}

bool metacg::io::VersionThreeMetaCGReader::isV3DebugFormat(const json &j) {
  // If no nodes exist, we have no IDs to change, and we also can not have edges
  if (j.at("nodes").is_null() || j.at("nodes").empty()) {
    return false;
  }
  // If the name identifiers are strings, we are in debug format
  return j["nodes"].at(0).at(0).is_string();
}

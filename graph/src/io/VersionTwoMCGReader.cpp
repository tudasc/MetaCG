
/**
 * File: VersionTwoMCGReader.cpp
 * License: Part of the MetaCG project. Licensed under BSD 3 clause license. See LICENSE.txt file at
 * https://github.com/tudasc/metacg/LICENSE.txt
 */
#include "io/VersionTwoMCGReader.h"
#include "MCGBaseInfo.h"
#include "Timing.h"
#include "Util.h"

std::unique_ptr<metacg::Callgraph> metacg::io::VersionTwoMetaCGReader::read() {
  const metacg::RuntimeTimer rtt("VersionTwoMetaCGReader::read");
  const metacg::MCGFileFormatInfo ffInfo{2, 0};
  auto console = metacg::MCGLogger::instance().getConsole();
  auto errConsole = metacg::MCGLogger::instance().getErrConsole();

  auto j = source.get();

  if (j.is_null()) {
    const std::string errorMsg = "JSON source did not contain any data.";
    errConsole->error(errorMsg);
    throw std::runtime_error(errorMsg);
  }

  if (!j.contains(ffInfo.metaInfoFieldName) || j.at(ffInfo.metaInfoFieldName).is_null()) {
    const std::string errorMsg = "Could not read version info from metacg file.";
    errConsole->error(errorMsg);
    throw std::runtime_error(errorMsg);
  }

  auto mcgInfo = j.at(ffInfo.metaInfoFieldName);
  // from here on we assume, that if any file meta information is given, it is correct

  /// XXX How to make that we can use the MCGGeneratorVersionInfo to access the identifiers
  auto mcgVersion = mcgInfo.at("version").get<std::string>();

  if (mcgVersion.compare(0, 1, std::string("2")) != 0) {
    const std::string errorMsg = "File is of version " + mcgVersion + ", this reader handles version 2.x";
    errConsole->error(errorMsg);
    throw std::runtime_error(errorMsg);
  }

  auto generatorName = mcgInfo.at("generator").at("name").get<std::string>();
  auto generatorVersion = mcgInfo.at("generator").at("version").get<std::string>();
  const MCGGeneratorVersionInfo genVersionInfo{generatorName, metacg::util::getMajorVersionFromString(generatorVersion),
                                               metacg::util::getMinorVersionFromString(generatorVersion), ""};
  console->info("The metacg (version {}) file was generated with {} (version: {})", mcgVersion, generatorName,
                generatorVersion);

  MCGFileInfo fileInfo{ffInfo, genVersionInfo};
  if (!j.contains(ffInfo.cgFieldName) || j.at(ffInfo.cgFieldName).is_null()) {
    const std::string errorMsg = "The call graph in the metacg file was not found or null.";
    errConsole->error(errorMsg);
    throw std::runtime_error(errorMsg);
  }

  auto& jsonCG = j[ffInfo.cgFieldName];
  upgradeV2FormatToV3Format(jsonCG);
  return std::make_unique<metacg::Callgraph>(jsonCG);
}

void metacg::io::VersionTwoMetaCGReader::upgradeV2FormatToV3Format(nlohmann::json& j) {
  // Add empty node container
  j["nodes"] = nlohmann::json::array();
  // Move nodes one layer deeper into node container
  for (auto it = j.items().begin(); it != j.items().end();) {
    if (it.key() == "nodes") {
      it.operator++();
      continue;
    }
    j.at("nodes").push_back({it.key(), std::move(it.value())});
    auto temp_It = it;
    it.operator++();
    j.erase(temp_It.key());
  }
  // Add empty edge container
  j["edges"] = nlohmann::json::array();

  // Swap function name with hash id
  for (auto& node : j["nodes"]) {
    const std::string functionName = node.at(0);

    // if by chance the V2 format contained origin data
    // calculate the hash with known origin
    // if the origin metadata exists, but is empty use unknownOrigin instead
    if (node.at(1).at("meta").contains("fileProperties") &&
        !node.at(1).at("meta").at("fileProperties").at("origin").get<std::string>().empty()) {
      node.at(1)["origin"] = node.at(1).at("meta").at("fileProperties").at("origin");
    } else {  // if the V2 format did not contain origin data use unknownOrigin keyword
      node.at(1)["origin"] = "unknownOrigin";
    }
    node.at(0) = std::hash<std::string>()(functionName + node.at(1)["origin"].get<std::string>());
    node.at(1)["functionName"] = functionName;
  }

  // populate edge container and overwrites
  for (auto& node : j["nodes"]) {
    // edges
    for (const auto& callee : node.at(1).at("callees")) {
      for (const auto& calleeNode : j["nodes"]) {
        if (calleeNode.at(1).at("functionName") == callee) {
          assert(!calleeNode.at(1).at("origin").get<std::string>().empty());
          j["edges"].push_back({{node.at(0), calleeNode.at(0)},{}});
          break;
        }
      }
    }
    node.at(1).erase("callees");
    node.at(1).erase("callers");

    // if the V2 format node was virtual, we add override metadata
    if (node.at(1).at("isVirtual")) {
      nlohmann::json overrideHashes = nlohmann::json::array();
      nlohmann::json overriddenByHashes = nlohmann::json::array();
      for (const auto& nodeCandidate : j["nodes"]) {
        for (const auto& overriddenNodeName : node.at(1).at("overrides")) {
          //Format V2 can not handle duplicate node names
          //We therefore assume functionName to be unique in a V2 file
          //This means we can break after finding the first correctly named entry
          if (nodeCandidate.at(1).at("functionName") == overriddenNodeName) {
            overrideHashes.push_back(nodeCandidate.at(0));
            break;
          }
        }
        for (const auto& n : node.at(1).at("overriddenBy")) {
          if (nodeCandidate.at(1).at("functionName") == n) {
            overriddenByHashes.push_back(nodeCandidate.at(0));
            break;
          }
        }
      }
      node.at(1)["meta"]["overrideMD"] = {{"overrides", overrideHashes}, {"overriddenBy", overriddenByHashes}};
    }
    node.at(1).erase("isVirtual");
    node.at(1).erase("doesOverride");
    node.at(1).erase("overrides");
    node.at(1).erase("overriddenBy");
  }
}

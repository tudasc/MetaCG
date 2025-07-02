/**
 * File: VersionFourMCGReader.cpp
 * License: Part of the MetaCG project. Licensed under BSD 3 clause license. See LICENSE.txt file at
 * https://github.com/tudasc/metacg/LICENSE.txt
 */

#include "io/VersionFourMCGReader.h"
#include "MCGBaseInfo.h"
#include "Timing.h"
#include "Util.h"
#include <iostream>


using namespace metacg;



namespace {

struct V4StrToNodeMapping : public io::StrToNodeMapping {
  CgNode* getNodeFromStr(const std::string& nodeStr) override {
    if (auto it = strToNode.find(nodeStr); it != strToNode.end()) {
      return it->second;
    }
    return nullptr;
  }

  bool registerNode(const std::string& nodeStr, CgNode& node) {
    if (auto it = strToNode.find(nodeStr); it != strToNode.end()) {
      metacg::MCGLogger::logError("Found duplicate node ID in the provided MetaCG file.");
      return false;
    }
    strToNode[nodeStr] = &node;
    return true;
  }
 private:
  std::unordered_map<std::string, CgNode*> strToNode;
};


metacg::CgNode* createNodeFromJson(metacg::Callgraph& cg, const std::string& nodeStr, const nlohmann::json& j, V4StrToNodeMapping& strToNode) {
  if (j.is_null()) {
    metacg::MCGLogger::logError("Node is null!");
    return nullptr;
  }
  std::optional<std::string> origin{};
  if (!j.contains("origin")) {
    metacg::MCGLogger::logError("Node must contain 'origin' field. Use 'null' to indicate unknown origin.");
    return nullptr;
  }
  if (!j.at("origin").is_null()) {
    if (!j.at("origin").empty()) {
      origin = j.at("origin");
    } else {
      metacg::MCGLogger::logWarnUnique("Encountered empty origin field. Please use an explicit 'null' value to indicate unknown origin.");
    }
  }
  if (!j.contains("functionName")) {
    metacg::MCGLogger::logError("Node must contain 'functionName' field");
    return nullptr;
  }
  auto& cgNode = cg.insert(j.at("functionName"), origin);
  if (!j.contains("hasBody")) {
    metacg::MCGLogger::logError("Node must contain 'hasBody' field");
    return nullptr;
  }
  cgNode.setHasBody(j.at("hasBody").get<bool>());
  if (!j.contains("meta")) {
    metacg::MCGLogger::logError("Node must contain 'meta' field");
    return nullptr;
  }
  cgNode.setMetaDataContainer(j.at("meta"));
  if (!strToNode.registerNode(nodeStr, cgNode)) {
    metacg::MCGLogger::logError("Faulty MetaCG file. Remove duplicate node identifiers to fix issue.");
    return nullptr;
  }
  return &cgNode;
}

/**
 * Internal data structure to temporarily store edge data for later processing.
 */
struct TempEdgeData {
  TempEdgeData(NodeId id, nlohmann::json& j) : callerId(id), j(j) {};
  NodeId callerId;
  nlohmann::json& j;
};


}

std::unique_ptr<metacg::Callgraph> metacg::io::VersionFourMetaCGReader::read() {
  const metacg::RuntimeTimer rtt("VersionFourMetaCGReader::read");
  const MCGFileFormatInfo ffInfo{4, 0};
  auto console = metacg::MCGLogger::instance().getConsole();
  auto errConsole = metacg::MCGLogger::instance().getErrConsole();

  auto j = source.get();
  auto mcgInfo = j[ffInfo.metaInfoFieldName];
  if (mcgInfo.is_null()) {
    errConsole->error("Could not read version info from metacg file.");
    throw std::runtime_error("Could not read version info from metacg file");
  }

  auto mcgVersion = mcgInfo["version"].get<std::string>();
  auto generatorName = mcgInfo["generator"]["name"].get<std::string>();
  auto generatorVersion = mcgInfo["generator"]["version"].get<std::string>();
  const MCGGeneratorVersionInfo genVersionInfo{generatorName, metacg::util::getMajorVersionFromString(generatorVersion),
                                               metacg::util::getMinorVersionFromString(generatorVersion), ""};
  console->info("The MetaCG (version {}) file was generated with {} (version: {})", mcgVersion, generatorName,
                generatorVersion);

  if (mcgVersion.at(0) != '4') {
    errConsole->error("This reader can only read MetaCG v4 files");
    throw std::runtime_error("Trying to read incompatible file with V4 MCGReader");
  }
  const MCGFileInfo fileInfo{ffInfo, genVersionInfo};
  auto& jsonCG = j[ffInfo.cgFieldName];
  if (jsonCG.is_null()) {
    errConsole->error("The call graph in the MetaCG file was null.");
    throw std::runtime_error("CG in MetaCG file was null.");
  }

  auto cg = std::make_unique<Callgraph>();

  std::vector<TempEdgeData> tempEdgeData;
  // Rough estimate of required size
  tempEdgeData.reserve(cg->size());

  V4StrToNodeMapping strToNode;

  for (nlohmann::json::iterator it = jsonCG.begin(); it != jsonCG.end(); ++it) {
    auto& strId  = it.key();
    auto& jNode = it.value();

    auto* node = createNodeFromJson(*cg, strId, jNode, strToNode);
    if (!node) {
      errConsole->error("Encountered an error while processing node '{}'", strId);
      throw std::runtime_error("Error while reading nodes");
    }
    // Save edges for faster processing
    for (auto& jEdge : jNode["edges"]) {
      tempEdgeData.emplace_back(node->getId(), jEdge);
    }
  }

  for (auto& nodeEdges : tempEdgeData) {
    for (nlohmann::json::iterator it = nodeEdges.j.begin(); it != nodeEdges.j.end(); ++it) {
      auto& calleeStr = it.key();
      auto& mdJ = it.value();
      auto* node = strToNode.getNodeFromStr(calleeStr);
      if (!node) {
        errConsole->error("Encountered unknown call target '{}' in edge from node '{}'", calleeStr, cg->getNode(nodeEdges.callerId)->getFunctionName());
        throw std::runtime_error("Error while reading edges");
      }
      cg->addEdge(nodeEdges.callerId, node->getId());
      // Reading edge metadata
      if (!mdJ.is_null()) {
        for (const auto& mdElem : mdJ.items()) {
          auto& mdKey = mdElem.key();
          auto& mdValJ = mdElem.value();
          if (auto md = metacg::MetaData::create<>(mdKey, mdValJ); md) {
            cg->addEdgeMetaData({nodeEdges.callerId, node->getId()}, std::move(md));
          }
        }
      }
    }
  }

  return cg;
}

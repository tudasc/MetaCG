/**
 * File: VersionFourMCGReader.cpp
 * License: Part of the MetaCG project. Licensed under BSD 3 clause license. See LICENSE.txt file at
 * https://github.com/tudasc/metacg/LICENSE.txt
 */

#include "io/VersionFourMCGReader.h"
#include "MCGBaseInfo.h"
#include "Timing.h"
#include "Util.h"
#include "metadata/BuiltinMD.h"
#include <iostream>

using namespace metacg;

namespace {

struct V4StrToNodeMapping : public StrToNodeMapping {
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

metacg::CgNode* createNodeFromJson(metacg::Callgraph& cg, const std::string& nodeStr, const nlohmann::json& j,
                                   V4StrToNodeMapping& strToNode) {
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
      metacg::MCGLogger::logWarnUnique(
          "Encountered empty origin field. Please use an explicit 'null' value to indicate unknown origin.");
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

  // Note: Checking the validity of the metadata field here, but actual processing is deferred until after ID mapping
  // is finalized.
  if (!j.contains("meta")) {
    metacg::MCGLogger::logError("Node must contain 'meta' field");
    return nullptr;
  }
  if (!j.at("meta").is_null()) {
    if (!j.at("meta").is_object()) {
      metacg::MCGLogger::logError("'meta' field must be an object");
      return nullptr;
    }
  }

  if (!strToNode.registerNode(nodeStr, cgNode)) {
    metacg::MCGLogger::logError("Faulty MetaCG file. Remove duplicate node identifiers to fix issue.");
    return nullptr;
  }
  return &cgNode;
}

/**
 * Internal data structure to temporarily store edges and metadata data for later processing.
 */
struct TempNodeData {
  TempNodeData(NodeId id, nlohmann::json& jEdges, nlohmann::json& jMeta) : nodeId(id), jEdges(jEdges), jMeta(jMeta) {};
  NodeId nodeId;
  nlohmann::json& jEdges;
  nlohmann::json& jMeta;
};

}  // namespace

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

  std::vector<TempNodeData> tempNodeData;
  // Rough estimate of required size
  tempNodeData.reserve(jsonCG.size());

  V4StrToNodeMapping strToNode;

  for (auto it = jsonCG.begin(); it != jsonCG.end(); ++it) {
    auto& strId = it.key();
    auto& jNode = it.value();

    auto* node = createNodeFromJson(*cg, strId, jNode, strToNode);
    if (!node) {
      errConsole->error("Encountered an error while processing node '{}'", strId);
      throw std::runtime_error("Error while reading nodes");
    }

    // Save edges and metadata for processing after IDs have been finalized
    tempNodeData.emplace_back(node->getId(), jNode["callees"], jNode["meta"]);
  }

  for (auto& nodeData : tempNodeData) {
    auto* node = cg->getNode(nodeData.nodeId);
    assert(node && "Node must not be null");
    // Creating edges
    for (auto it = nodeData.jEdges.begin(); it != nodeData.jEdges.end(); ++it) {
      auto& calleeStr = it.key();
      auto& mdJ = it.value();
      auto* calleeNode = strToNode.getNodeFromStr(calleeStr);
      if (!calleeNode) {
        errConsole->error("Encountered unknown call target '{}' in edge from node '{}'", calleeStr,
                          node->getFunctionName());
        throw std::runtime_error("Error while reading edges");
      }
      cg->addEdge(nodeData.nodeId, calleeNode->getId());
      // Reading edge metadata
      if (!mdJ.is_null()) {
        for (const auto& mdElem : mdJ.items()) {
          auto& mdKey = mdElem.key();
          auto& mdValJ = mdElem.value();
          if (auto md = metacg::MetaData::create<>(mdKey, mdValJ, strToNode); md) {
            cg->addEdgeMetaData({nodeData.nodeId, calleeNode->getId()}, std::move(md));
          }
        }
      }
    }
    // Creating node metadata
    for (auto it = nodeData.jMeta.begin(); it != nodeData.jMeta.end(); ++it) {
      auto& mdKey = it.key();
      auto& mdVal = it.value();
      if (auto md = metacg::MetaData::create<>(mdKey, mdVal, strToNode); md) {
        node->addMetaData(std::move(md));
      } else {
        errConsole->warn("Could not create metadata of type {} for node {}", mdKey, node->getFunctionName());
      }
    }
  }

  return cg;
}

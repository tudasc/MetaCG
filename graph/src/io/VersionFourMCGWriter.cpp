/**
 * File: VersionFourMCGWriter.cpp
 * License: Part of the MetaCG project. Licensed under BSD 3 clause license. See LICENSE.txt file at
 * https://github.com/tudasc/metacg/LICENSE.txt
 */

#include "io/VersionFourMCGWriter.h"
#include "MCGManager.h"
#include "config.h"
#include <iostream>

using namespace metacg;


namespace {

struct V4WriterMapping: public io::NodeToStrMapping {

  V4WriterMapping(const Callgraph& cg, bool useNameAsId) : cg(cg), useNameAsId(useNameAsId) {
  }

  std::string getStrFromNode(const CgNode& node) override {
    if (auto it = idToStr.find(node.getId()); it == idToStr.end()) {
      return it->second;
    }
    idToStr[node.getId()] = std::move(convertToStr(node));
    return idToStr[node.getId()];
  }

 private:
  std::string convertToStr(const CgNode& node) {
    std::string idStr;
    if (useNameAsId) {
      auto& allNodesWithName = cg.getNodes(node.getFunctionName());
      idStr = node.getFunctionName();
      if (allNodesWithName.size() > 1) {
        auto it = std::find(allNodesWithName.begin(), allNodesWithName.end(), node.getId());
        auto idx = std::distance(allNodesWithName.begin(), it);
        // Using '#' because it is not valid in an ELF symbol name. This ensures that we do not accidentally use the name
        // of another, unrelated symbol.
        idStr += "#" + std::to_string(static_cast<int>(idx));
      }
    } else {
      idStr = std::to_string(node.getId());
    }
    return idStr;
  }

 private:
  const Callgraph& cg;
  bool useNameAsId;
  std::unordered_map<NodeId, std::string> idToStr;
};

// A node cannot serialize/deserialize itself, as it is dependent on the containing call graph to assign IDs.
// Therefore, we have to this manually.
nlohmann::json createJsonFromNode(const CgNode& node, io::NodeToStrMapping& nodeToStr) {

  auto idStr = nodeToStr.getStrFromNode(node);

  nlohmann::json j = {idStr, {{"functionName", node.getFunctionName()},
       {"origin", node.getOrigin()},
       {"hasBody", node.getHasBody()},
       /*{"meta", node.getMetaDataContainer()}*/}};

  return j;
}


}

void metacg::io::VersionFourMCGWriter::write(const metacg::Callgraph* cg, metacg::io::JsonSink& js) {
  nlohmann::json j;
  attachMCGFormatHeader(j);

  nlohmann::json jNodes{};

  V4WriterMapping nodeToStr(*cg, useNamesAsIds);

  // First serialize all nodes, then insert the edges.
  auto& nodes = cg->getNodes();
  for (auto& node : nodes) {
    if (!node) {
      continue;
    }
    nlohmann::json jNode =  {{"functionName", node->getFunctionName()},
                                {"origin", node->getOrigin()},
                                {"hasBody", node->getHasBody()},
                                {"edges", {}},
                                /*{"meta", node.getMetaDataContainer()}*/};
    // TOOD: Add node MD
    auto idStr = nodeToStr.getStrFromNode(*node);
    jNodes[idStr] = std::move(jNode);
  }

  // Now insert the edges.
  auto& edges = cg->getEdges();
  for (auto& edge : edges) {
    auto* caller = cg->getNode(edge.first.first);
    auto* callee = cg->getNode(edge.first.second);
    assert(caller && callee && "Encountered invalid edge");
    auto callerStr = nodeToStr.getStrFromNode(*caller);
    auto calleeStr = nodeToStr.getStrFromNode(*callee);

    nlohmann::json jEdgeMD = {};
    auto& mdMap = cg->getAllEdgeMetaData({caller->getId(), callee->getId()});
    for (auto&& [key, val] : mdMap) {
      jEdgeMD[key] = val->to_json();
    }

    jNodes[callerStr]["edges"].push_back({calleeStr, jEdgeMD});
  }

  j["_CG"] = std::move(jNodes);

  if (exportSorted) {
    sortCallgraph(j);
  }
  js.setJson(j);
}

void metacg::io::VersionFourMCGWriter::sortCallgraph(nlohmann::json& j) {
  // Assume we get only the graph
  nlohmann::json* callgraph = &j;
  // If we get the whole container, extract the graph
  if (j.contains("_CG")) {
    callgraph = &j["_CG"];
  }

  for (auto& elem : *callgraph) {
    if (elem.is_array()) {
      std::sort(elem.begin(), elem.end());
    }
  }
}


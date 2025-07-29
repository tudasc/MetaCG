/**
 * File: VersionFourMCGWriter.cpp
 * License: Part of the MetaCG project. Licensed under BSD 3 clause license. See LICENSE.txt file at
 * https://github.com/tudasc/metacg/LICENSE.txt
 */

#include "io/VersionFourMCGWriter.h"
#include "MCGManager.h"
#include "config.h"
#include "metadata/BuiltinMD.h"
#include <iostream>

using namespace metacg;

namespace {

struct V4WriterMapping : public NodeToStrMapping {
  // Note: The following allows overload resolution of the base class function.
  using NodeToStrMapping::getStrFromNode;

  V4WriterMapping(const Callgraph& cg, bool useNameAsId) : cg(cg), useNameAsId(useNameAsId) {}

  std::string getStrFromNode(NodeId id) override {
    if (auto it = idToStr.find(id); it != idToStr.end()) {
      return it->second;
    }
    assert(cg.getNode(id) && "ID must be valid");
    idToStr[id] = std::move(convertToStr(*cg.getNode(id)));
    return idToStr[id];
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
        // Using '#' because it is not valid in an ELF symbol name. This ensures that we do not accidentally use the
        // name of another, unrelated symbol.
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

}  // namespace

void metacg::io::VersionFourMCGWriter::write(const metacg::Callgraph* cg, metacg::io::JsonSink& js) {
  nlohmann::json j;
  attachMCGFormatHeader(j);

  nlohmann::json jNodes = nlohmann::json::object();

  V4WriterMapping nodeToStr(*cg, useNamesAsIds);

  // First serialize all nodes, then insert the edges.
  auto& nodes = cg->getNodes();
  for (auto& node : nodes) {
    if (!node) {
      continue;
    }
    auto jMeta = nlohmann::json::object();

    for (auto& [key, md] : node->getMetaDataContainer()) {
      //        std::cout << "Processing MD " << key << "\n"; //FIXME: remove
      // Metadata is not attached, if the generated field is empty or null.
      // TODO: Should this be considered an error instead?
      if (auto jMetaEntry = md->toJson(nodeToStr); !jMetaEntry.empty() && !jMetaEntry.is_null()) {
        jMeta[key] = std::move(jMetaEntry);
      } else {
        MCGLogger::logWarn("Could not serialize metadata of type {} in node {}", key, node->getFunctionName());
      }
    }

    nlohmann::json jNode = {{"functionName", node->getFunctionName()},
                            {"origin", node->getOrigin()},
                            {"hasBody", node->getHasBody()},
                            {"callees", nlohmann::json::object()},
                            {"meta", jMeta}};

    //    std::cout << "V4 generated node json: " << jNode << "\n"; // FIXME: remove

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

    nlohmann::json jEdgeMD = nlohmann::json::object();
    auto& mdMap = cg->getAllEdgeMetaData({caller->getId(), callee->getId()});
    for (auto&& [key, val] : mdMap) {
      jEdgeMD[key] = val->toJson(nodeToStr);
    }

    jNodes[callerStr]["callees"][calleeStr] = jEdgeMD;
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

/**
 * File: NodeDiff.h
 * License: Part of the MetaCG project. Licensed under BSD 3 clause license. See LICENSE.txt file at
 * https://github.com/tudasc/metacg/LICENSE.txt
 */

#pragma once

#include "Diff.h"
#include "NodeSummary.h"
#include "nlohmann/json_fwd.hpp"
#include <string>

struct NodeDiff : Diff {
  struct EdgeDiff {
    std::string callee;
    bool onlyInA = false;
    bool onlyInB = false;
    std::unordered_set<std::string> metadataOnlyInA;
    std::unordered_set<std::string> metadataOnlyInB;
  };

  NodeDiff(std::string name, std::vector<std::string> diffType = {}, std::vector<EdgeDiff> edgeDiffs = {},
           std::unordered_set<std::string> metaA = {}, std::unordered_set<std::string> metaB = {})
      : name(std::move(name)),
        diffType(std::move(diffType)),
        edgeDiffs(std::move(edgeDiffs)),
        metadataOnlyInA(std::move(metaA)),
        metadataOnlyInB(std::move(metaB)) {}


  std::string name;
  std::vector<std::string> diffType;
  std::string in;
  std::vector<EdgeDiff> edgeDiffs;
  std::unordered_set<std::string> metadataOnlyInA;
  std::unordered_set<std::string> metadataOnlyInB;

  /**
   * Factory function for a node that exists only in the first graph (A).
   *
   * @param a NodeSummary from the first graph
   * @return NodeDiff representing a missing node in A
   */
  static NodeDiff onlyInA(const NodeSummary& a) {
    NodeDiff nd = NodeDiff(a.name, {"missingNode"}, {}, {}, {});
    nd.in = "cgA";
    return nd;
  }

  /**
   * Factory function for a node that exists only in the first graph (B).
   *
   * @param a NodeSummary from the first graph
   * @return NodeDiff representing a missing node in B
   */
  static NodeDiff onlyInB(const NodeSummary& b) {
    NodeDiff nd = NodeDiff(b.name, {"missingNode"}, {}, {}, {});
    nd.in = "cgB";
    return nd;
  }

  virtual Kind kind() const override { return Kind::Node; }

  virtual nlohmann::ordered_json toJson() const override {
    nlohmann::ordered_json inner;
    inner["diffType"] = diffType;
    inner["in"] = in.empty() ? "both" : in;
    inner["metadataOnlyInA"] = metadataOnlyInA;
    inner["metadataOnlyInB"] = metadataOnlyInB;

    if (!edgeDiffs.empty()) {
      nlohmann::ordered_json edgesJson = nlohmann::ordered_json::object();
      for (const auto& ed : edgeDiffs) {
        nlohmann::ordered_json edgeInfo;
        edgeInfo["in"] = ed.onlyInA ? "cgA" : (ed.onlyInB ? "cgB" : "both");
        edgeInfo["metadataOnlyInA"] = ed.metadataOnlyInA;
        edgeInfo["metadataOnlyInB"] = ed.metadataOnlyInB;
        edgesJson[ed.callee] = edgeInfo;
      }
      inner["edges"] = edgesJson;
    }
    return inner;
  }
};

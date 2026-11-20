/**
 * File: MergePolicy.cpp
 * License: Part of the MetaCG project. Licensed under BSD 3 clause license. See LICENSE.txt file at
 * https://github.com/tudasc/metacg/LICENSE.txt
 */
#include "metacg/MergePolicy.h"

#include "metacg/Callgraph.h"
#include "metacg/LoggerUtil.h"

namespace metacg {

std::optional<MergeAction> MergeByName::findMatchingNode(const Callgraph& targetCG, const CgNode& sourceNode) const {
  auto& nameMatches = targetCG.getAllNodesWithName(sourceNode.getFunctionName());
  if (nameMatches.empty()) {
    // There is no matching node: simply copy the source node into the target graph
    return {};
  }
  if (nameMatches.size() > 1) {
    // There is more than one node: in V2, this can't happen. Print a warning and merge with the first node.
    MCGLogger::logWarn("Found multiple nodes with matching name '{}'! Merging with the first node.",
                       sourceNode.getFunctionName());
  }
  auto* targetNode = targetCG.getNode(nameMatches.front());
  assert(targetNode && "Node must not be null");

  // If the source node has no body (i.e. no definition) OR the target node has one, keep the target node.
  // If both functions have bodies: assume inline with identical function bodies.
  if (targetNode->getHasBody() || !sourceNode.getHasBody()) {
    return MergeAction(targetNode->getId(), false);
  }

  return MergeAction(targetNode->getId(), true);
}

std::optional<MergeAction> MergeByNameAndOrigin::findMatchingNode(const Callgraph& targetCG,
                                                                  const CgNode& sourceNode) const {
  auto& nameMatches = targetCG.getAllNodesWithName(sourceNode.getFunctionName());
  if (nameMatches.empty()) {
    // There is no matching node: simply copy the source node into the target graph
    return {};
  }

  // Among the nodes with a matching name, find those that also have the same origin.
  // Note: two nodes without an origin (std::nullopt) are considered to have the same origin.
  const auto sourceOrigin = sourceNode.getOrigin();
  CgNode* targetNode = nullptr;
  size_t numMatches = 0;
  for (const auto nodeId : nameMatches) {
    auto* candidate = targetCG.getNode(nodeId);
    assert(candidate && "Node must not be null");
    if (candidate->getOrigin() == sourceOrigin) {
      if (!targetNode) {
        targetNode = candidate;
      }
      ++numMatches;
    }
  }

  if (!targetNode) {
    // Nodes with the same name exist, but none with the same origin: copy the source node into the target graph
    return {};
  }
  if (numMatches > 1) {
    MCGLogger::logWarn("Found multiple nodes with matching name '{}' and origin '{}'! Merging with the first node.",
                       sourceNode.getFunctionName(), sourceOrigin.value_or("<none>"));
  }

  // If the source node has no body (i.e. no definition) OR the target node has one, keep the target node.
  // If both functions have bodies: assume inline with identical function bodies.
  if (targetNode->getHasBody() || !sourceNode.getHasBody()) {
    return MergeAction(targetNode->getId(), false);
  }

  return MergeAction(targetNode->getId(), true);
}

}  // namespace metacg

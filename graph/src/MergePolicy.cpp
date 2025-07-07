/**
* File: MergePolicy.cpp
* License: Part of the MetaCG project. Licensed under BSD 3 clause license. See LICENSE.txt file at
* https://github.com/tudasc/metacg/LICENSE.txt
 */
#include "MergePolicy.h"

#include "Callgraph.h"
#include "LoggerUtil.h"

namespace metacg {

std::optional<MergeAction> MergeByName::findMatchingNode(const Callgraph& targetCG, const CgNode& sourceNode) const {
  auto& nameMatches = targetCG.getNodes(sourceNode.getFunctionName());
  if (nameMatches.empty()) {
    // There is no matching node: simply copy the source node into the target graph
    return {};
  }
  if (nameMatches.size() > 1) {
    // There is more than one node: in V2, this can't happen. Print a warning and merge with the first node.
    MCGLogger::logWarn("Found multiple nodes with matching name '{}'! Merging with the first node.", sourceNode.getFunctionName());
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

}

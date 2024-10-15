/**
 * File: ReachabilityAnalysis.cpp
 * License: Part of the MetaCG project. Licensed under BSD 3 clause license. See LICENSE.txt file at
 * https://github.com/tudasc/metacg/LICENSE.txt
 */

#include "ReachabilityAnalysis.h"

#include <queue>

namespace metacg::analysis {

void ReachabilityAnalysis::runForNode(const CgNode* const n) {
  computedFor.insert(n);

  auto& reachableSet = reachableNodes[n];
  // Compute the information as it is not available
  std::unordered_set<const CgNode*> visitedNodes;
  std::queue<const CgNode*> workQueue;
  workQueue.push(n);

  // Visit all reachable nodes and mark as visited
  while (!workQueue.empty()) {
    auto node = workQueue.front();
    visitedNodes.insert(node);
    workQueue.pop();

    // children need to be processed
    // XXX include knowledge if childNode is in computedFor set
    for (const auto childNode : cg->getCallees(node->getId())) {
      if (visitedNodes.find(childNode) == visitedNodes.end()) {
        workQueue.push(childNode);
      }
    }
  }

  // Move result to instance-local cache.
  std::swap(visitedNodes, reachableSet);
}

void ReachabilityAnalysis::computeReachableFromMain() {
  const auto mainNode = cg->getMain();
  assert(mainNode != nullptr && "Needs to have main node");
  runForNode(mainNode);
}

bool ReachabilityAnalysis::isReachableFromMain(const CgNode* const node, bool forceUpdate) {
  if (forceUpdate || computedFor.find(cg->getMain()) == computedFor.end()) {
    computeReachableFromMain();
  }
  auto reachableFromMain = reachableNodes.find(cg->getMain());
  return reachableFromMain->second.find(node) != reachableFromMain->second.end();
}

bool ReachabilityAnalysis::existsPathBetween(const CgNode* const src, const CgNode* const dest, bool forceUpdate) {
  auto& reachableSet = reachableNodes[src];
  // Check if we already computed for src and return if we found that a path exists
  if (!forceUpdate && computedFor.find(src) != computedFor.end()) {
    return reachableSet.find(dest) != reachableSet.end();
  }

  runForNode(src);
  // XXX Is the reachableSet reference actually updated to hold potentially newly created nodes?

  return reachableSet.find(dest) != reachableSet.end();
}

}  // namespace metacg::analysis
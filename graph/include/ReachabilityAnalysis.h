/**
 * File: ReachabilityAnalysis.h
 * License: Part of the metacg project. Licensed under BSD 3 clause license. See LICENSE.txt file at
 * https://github.com/tudasc/metacg/LICENSE.txt
 */

#ifndef METACG_REACHABILITYANALYSIS_H
#define METACG_REACHABILITYANALYSIS_H

#include "Callgraph.h"

#include <unordered_map>
#include <unordered_set>

namespace metacg::analysis {

/**
 * Provides reachability analysis information for graph.
 * TODO: What does "path between A and A" mean in our analysis?
 */
class ReachabilityAnalysis {
 public:
  explicit ReachabilityAnalysis(Callgraph* graph) : cg(graph) {}

  /** Pre-computes all nodes reachable from 'main' function. */
  void computeReachableFromMain();
  /** Query if node is reachable from main */
  bool isReachableFromMain(const CgNode* const node, bool forceUpdate = false);
  /** Compute if path exists between any two nodes in graph */
  bool existsPathBetween(const CgNode* const src, const CgNode* const dest, bool forceUpdate = false);

 private:
  void runForNode(const CgNode* const n);

  Callgraph* cg;
  std::unordered_map<const CgNode*, std::unordered_set<const CgNode*>> reachableNodes;
  std::unordered_set<const CgNode*> computedFor;  // cache searched nodes
};

}  // namespace metacg::analysis
#endif  // METACG_REACHABILITYANALYSIS_H

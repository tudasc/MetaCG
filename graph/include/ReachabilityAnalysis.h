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
 */
class ReachabilityAnalysis {
 public:
  explicit ReachabilityAnalysis(Callgraph *graph) : cg(graph) {}

  /** Pre-computes all nodes reachable from 'main' function. */
  void computeReachableFromMain();
  /** Query if node is reachable from main */
  bool isReachableFromMain(CgNodePtr node, bool forceUpdate = false);
  /** Compute if path exists between any two nodes in graph */
  bool existsPathBetween(CgNodePtr src, CgNodePtr dest, bool forceUpdate = false);

 private:
  void runForNode(CgNodePtr n);

  Callgraph *cg;
  std::unordered_map<CgNodePtr, std::unordered_set<CgNodePtr>> reachableNodes;
  std::unordered_set<CgNodePtr> computedFor; // cache searched nodes
};

} // namespace metacg::analysis
#endif  // METACG_REACHABILITYANALYSIS_H

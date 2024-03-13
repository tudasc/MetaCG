/**
 * File: CgHelper.h
 * License: Part of the metacg project. Licensed under BSD 3 clause license. See LICENSE.txt file at
 * https://github.com/tudasc/metacg/LICENSE.txt
 */

#ifndef CGNODEHELPER_H_
#define CGNODEHELPER_H_

#include "Callgraph.h"
#include "CgNode.h"
#include "ReachabilityAnalysis.h"

#include <algorithm>  // std::set_intersection
#include <memory>
#include <queue>

#include <cassert>

namespace metacg {
class Callgraph;
}

struct Config {
  double actualRuntime = .0;
  double totalRuntime = .0;
  bool useMangledNames = true;
  std::string appName = "";

  std::string outputFile{"out"};
  std::string whitelist = "";
};

namespace CgHelper {

bool isConjunction(const metacg::CgNode *node, const metacg::Callgraph *const graph);

CgNodeRawPtrUSet getInstrumentationPath(metacg::CgNode *start, const metacg::Callgraph *const graph);

/**
 * Calculates the inclusive statement count for every node which is reachable from mainNode and saves in the node field
 */
void calculateInclusiveStatementCounts(metacg::CgNode *mainNode, const metacg::Callgraph *const graph);

CgNodeRawPtrUSet allNodesToMain(metacg::CgNode *startNode, metacg::CgNode *mainNode,
                                const metacg::Callgraph *const graph,
                                const std::unordered_map<metacg::CgNode *, CgNodeRawPtrUSet> &init,
                                metacg::analysis::ReachabilityAnalysis &ra);
CgNodeRawPtrUSet allNodesToMain(metacg::CgNode *startNode, metacg::CgNode *mainNode,
                                const metacg::Callgraph *const graph, metacg::analysis::ReachabilityAnalysis &ra);

CgNodeRawPtrUSet getDescendants(metacg::CgNode *child, const metacg::Callgraph *const graph);
CgNodeRawPtrUSet getAncestors(metacg::CgNode *child, const metacg::Callgraph *const graph);

/**
 *
 * @param cg
 * @param useLongAsRef If TRUE use half second biggest runtime, else median
 * @return
 */
double calcRuntimeThreshold(const metacg::Callgraph &cg, bool useLongAsRef);

inline CgNodeRawPtrUSet setIntersect(const CgNodeRawPtrUSet &a, const CgNodeRawPtrUSet &b) {
  CgNodeRawPtrUSet intersect;

  std::set_intersection(a.begin(), a.end(), b.begin(), b.end(), std::inserter(intersect, intersect.begin()));

  return intersect;
}

inline CgNodeRawPtrUSet setDifference(const CgNodeRawPtrUSet &a, const CgNodeRawPtrUSet &b) {
  CgNodeRawPtrUSet difference;

  std::set_difference(a.begin(), a.end(), b.begin(), b.end(), std::inserter(difference, difference.begin()));

  return difference;
}

inline bool isSubsetOf(const CgNodeRawPtrUSet &smallSet, const CgNodeRawPtrUSet &largeSet) {
  return setDifference(smallSet, largeSet) == smallSet;
}

inline bool intersects(const CgNodeRawPtrUSet &a, const CgNodeRawPtrUSet &b) { return !setIntersect(a, b).empty(); }

/**
 *
 * @param graph
 * @param node
 * @param calledFunctionName
 * @return Estimation of how often `node` calls `calledFunctionName` per invocation
 */
double getEstimatedCallsFromNode(metacg::Callgraph *graph, metacg::CgNode *node, const std::string &calledFunctionName);

}  // namespace CgHelper

namespace metacg::pgis {

using Callgraph = metacg::Callgraph;

template <typename MD, typename... Args>
void attachMetaDataToGraph(Callgraph *cg, const Args &...args) {
  for (const auto &n : cg->getNodes()) {
    if (!n.second->template getOrCreateMD<MD>(args...)) {
      assert(false && MD::key != "" && "Could not create MetaData with key");
    }
  }
}
}  // namespace metacg::pgis
#endif

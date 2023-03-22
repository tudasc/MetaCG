/**
 * File: CgHelper.cpp
 * License: Part of the metacg project. Licensed under BSD 3 clause license. See LICENSE.txt file at
 * https://github.com/tudasc/metacg/LICENSE.txt
 */

#include "CgHelper.h"
#include "../../../graph/include/Callgraph.h"
#include <loadImbalance/LIMetaData.h>

#include "spdlog/spdlog.h"
#include "MetaData/PGISMetaData.h"

using namespace metacg;

int CgConfig::samplesPerSecond = 10000;

namespace CgHelper {

using namespace pira;

/** returns true for nodes with two or more parents */
bool isConjunction(const metacg::CgNode* node, const metacg::Callgraph* const graph) { return (graph->getCallers(node).size() > 1); }

/** Returns a set of all nodes from the starting node up to the instrumented
 * nodes.
 *  It should not break for cycles, because cycles have to be instrumented by
 * definition. */
CgNodeRawPtrUSet getInstrumentationPath(metacg::CgNode* start, const metacg::Callgraph* const graph) {
  CgNodeRawPtrUSet path;  // visited nodes
  std::queue<metacg::CgNode*> workQueue;
  workQueue.push(start);

  while (!workQueue.empty()) {
    auto node = workQueue.front();
    workQueue.pop();

    path.insert(node);
    if (pgis::isInstrumented(node) || pgis::isInstrumentedPath(node) || /*node.isRootNode()*/ graph->getCallers(node).empty()) {
      continue;
    }

    for (auto parentNode : graph->getCallers(node)) {
      if (path.find(parentNode) == path.end()) {
        workQueue.push(parentNode);
      }
    }
  }

  return CgNodeRawPtrUSet (path.begin(), path.end());
}

bool isOnCycle(metacg::CgNode* node, const metacg::Callgraph* const graph) {
  CgNodeRawPtrUSet visitedNodes;
  std::queue<metacg::CgNode*> workQueue;
  workQueue.push(node);
  while (!workQueue.empty()) {
    auto currentNode = workQueue.front();
    workQueue.pop();

    if (visitedNodes.count(currentNode) == 0) {
      visitedNodes.insert(currentNode);

      for (auto child : graph->getCallees(currentNode)) {
        if (child == node) {
          return true;
        }
        workQueue.push(child);
      }
    }
  }
  return false;
}

Statements visitNodeForInclusiveStatements(metacg::CgNode* node, CgNodeRawPtrUSet *visitedNodes, const metacg::Callgraph* const graph) {
  if (visitedNodes->find(node) != visitedNodes->end()) {
    return node->get<LoadImbalance::LIMetaData>()->getNumberOfInclusiveStatements();
  }
  visitedNodes->insert(node);

  CgNodeRawPtrUSet visistedChilds;

  Statements inclusiveStatements = node->get<PiraOneData>()->getNumberOfStatements();
  for (auto childNode : graph->getCallees(node)) {
    // prevent double processing
    if (visistedChilds.find(childNode) != visistedChilds.end())
      continue;
    visistedChilds.insert(childNode);

    // approximate statements of a abstract function with maximum of its children (potential call targets)
    if (node->get<LoadImbalance::LIMetaData>()->isVirtual() && node->get<PiraOneData>()->getNumberOfStatements() == 0) {
      inclusiveStatements = std::max(inclusiveStatements, visitNodeForInclusiveStatements(childNode, visitedNodes,graph));
    } else {
      inclusiveStatements += visitNodeForInclusiveStatements(childNode, visitedNodes,graph);
    }
  }

  node->get<LoadImbalance::LIMetaData>()->setNumberOfInclusiveStatements(inclusiveStatements);

  metacg::MCGLogger::instance().getConsole()->trace("Visiting node " + node->getFunctionName() +
                                ". Result = " + std::to_string(inclusiveStatements));
  return inclusiveStatements;
}

void calculateInclusiveStatementCounts(metacg::CgNode* mainNode, const metacg::Callgraph* const graph) {
  CgNodeRawPtrUSet visitedNodes;

  metacg::MCGLogger::instance().getConsole()->trace("Starting inclusive statement counting. mainNode = " + mainNode->getFunctionName());

  visitNodeForInclusiveStatements(mainNode, &visitedNodes,graph);
}

CgNodeRawPtrUSet allNodesToMain(metacg::CgNode* startNode, metacg::CgNode* mainNode, const metacg::Callgraph* const graph,
                                     const std::unordered_map<metacg::CgNode*, CgNodeRawPtrUSet> &init, metacg::analysis::ReachabilityAnalysis &ra) {
  {
    auto it = init.find(startNode);
    if (it != init.end()) {
      return (*it).second;
    }
  }

  CgNodeRawPtrUSet pNodes;
  pNodes.insert(mainNode);

  CgNodeRawPtrUSet visitedNodes;
  std::queue<metacg::CgNode*> workQueue;
  workQueue.push(startNode);

  while (!workQueue.empty()) {
    auto node = workQueue.front();
    workQueue.pop();

    visitedNodes.insert(node);

    if (ra.isReachableFromMain(node)) {
      pNodes.insert(node);
    } else {
      continue;
    }

    auto pns = graph->getCallers(node);
    for (auto pNode : pns) {
      if (visitedNodes.find(pNode) == visitedNodes.end()) {
        workQueue.push(pNode);
      }
    }
  }

  return pNodes;
}

CgNodeRawPtrUSet allNodesToMain(metacg::CgNode* startNode, metacg::CgNode* mainNode, const metacg::Callgraph* const graph, metacg::analysis::ReachabilityAnalysis &ra) {
  return allNodesToMain(startNode, mainNode, graph,{}, ra);
}

/** Returns a set of all descendants including the starting node */
CgNodeRawPtrUSet getDescendants(metacg::CgNode* startingNode, const metacg::Callgraph* const graph) {
  // CgNodePtrUnorderedSet childs;
  CgNodeRawPtrUSet childs;
  std::queue<metacg::CgNode*> workQueue;
  workQueue.push(startingNode);

  while (!workQueue.empty()) {
    auto node = workQueue.front();
    workQueue.pop();

    childs.insert(node);

    for (auto childNode : graph->getCallees(node)) {
      if (childs.find(childNode) == childs.end()) {
        workQueue.push(childNode);
      }
    }
  }
  return childs;
}

/** Returns a set of all ancestors including the startingNode */
CgNodeRawPtrUSet getAncestors(metacg::CgNode* startingNode, const metacg::Callgraph* const graph) {
  CgNodeRawPtrUSet ancestors;
  std::queue<metacg::CgNode*> workQueue;
  workQueue.push(startingNode);

  while (!workQueue.empty()) {
    auto node = workQueue.front();
    workQueue.pop();

    ancestors.insert(node);

    for (auto parentNode : graph->getCallers(node)) {
      if (ancestors.find(parentNode) == ancestors.end()) {
        workQueue.push(parentNode);
      }
    }
  }

  return ancestors;
}

double calcRuntimeThreshold(const Callgraph &cg, bool useLongAsRef) {
  std::vector<double> rt;
  for (const auto &elem : cg.getNodes()) {
    const auto& n = elem.second.get();
    const auto &[hasBPD, bpd] = n->checkAndGet<BaseProfileData>();
    if (hasBPD) {
      metacg::MCGLogger::instance().getConsole()->trace("Found BaseProfileData for {}: Adding inclusive runtime of {} to RT vector.",
                                    n->getFunctionName(), bpd->getInclusiveRuntimeInSeconds());
      if (bpd->getInclusiveRuntimeInSeconds() != 0) {
        rt.push_back(bpd->getInclusiveRuntimeInSeconds());
      }
    }
  }
  metacg::MCGLogger::instance().getConsole()->info("The number of elements for runtime threshold calculation: {}", rt.size());

  std::sort(rt.begin(), rt.end());
  {  // raii
    std::string runtimeStr;
    for (const auto r : rt) {
      runtimeStr += ' ' + std::to_string(r);
    }
    metacg::MCGLogger::instance().getConsole()->debug("Runtime vector [values are seconds]: {}", runtimeStr);
  }

  size_t lastIndex = rt.size() * .5;
  if (useLongAsRef) {
    float runtimeFilterThresholdAlpha = 0.5f;
    lastIndex = rt.size() - 1;                           // TODO: Make this adjustable
    return rt[lastIndex] * runtimeFilterThresholdAlpha;  // runtime filtering threshold
  }
  // Returns the median of the data
  return rt[lastIndex];
}

}  // namespace CgHelper

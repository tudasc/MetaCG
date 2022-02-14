/**
 * File: CgHelper.cpp
 * License: Part of the metacg project. Licensed under BSD 3 clause license. See LICENSE.txt file at
 * https://github.com/tudasc/metacg/LICENSE.txt
 */

#include "CgHelper.h"
#include "../../../graph/include/Callgraph.h"
#include <loadImbalance/LIMetaData.h>

#include "spdlog/spdlog.h"

using namespace metacg;

int CgConfig::samplesPerSecond = 10000;

namespace CgHelper {

using namespace pira;

/** returns true for nodes with two or more parents */
bool isConjunction(CgNodePtr node) { return (node->getParentNodes().size() > 1); }

/** Returns a set of all nodes from the starting node up to the instrumented
 * nodes.
 *  It should not break for cycles, because cycles have to be instrumented by
 * definition. */
CgNodePtrSet getInstrumentationPath(CgNodePtr start) {
  CgNodePtrUnorderedSet path;  // visited nodes
  std::queue<CgNodePtr> workQueue;
  workQueue.push(start);

  while (!workQueue.empty()) {
    auto node = workQueue.front();
    workQueue.pop();

    path.insert(node);

    if (node->isInstrumented() || node->isRootNode()) {
      continue;
    }

    for (auto parentNode : node->getParentNodes()) {
      if (path.find(parentNode) == path.end()) {
        workQueue.push(parentNode);
      }
    }
  }

  return CgNodePtrSet(path.begin(), path.end());
}

/** returns the overhead caused by a call path */
// TODO this will only work with direct parents instrumented
unsigned long long getInstrumentationOverheadOfConjunction(CgNodePtr conjunctionNode) {
  auto parents = conjunctionNode->getParentNodes();

  CgNodePtrSet potentiallyInstrumented;
  for (auto parentNode : parents) {
    auto tmpSet = getInstrumentationPath(parentNode);
    potentiallyInstrumented.insert(tmpSet.begin(), tmpSet.end());
  }

  // add costs if node is instrumented
  return std::accumulate(potentiallyInstrumented.begin(), potentiallyInstrumented.end(), 0ULL,
                         [](unsigned long long acc, CgNodePtr node) {
                           if (node->isInstrumentedWitness()) {
                             auto nrCalls = 0ull;
                             if (node->has<BaseProfileData>()) {
                               nrCalls = node->get<BaseProfileData>()->getNumberOfCalls();
                             }
                             return acc + (nrCalls * CgConfig::nanosPerInstrumentedCall);
                           }
                           return acc;
                         });
}

bool isValidMarkerPosition(CgNodePtr markerPosition, CgNodePtr conjunction) {
  if (isOnCycle(markerPosition)) {
    return true;  // nodes on cycles are always valid marker positions
  }

  // if one parent of the conjunction parents is unreachable -> valid marker
  for (auto parentNode : conjunction->getParentNodes()) {
    if (!reachableFrom(markerPosition, parentNode)) {
      return true;
    }
  }
  return false;
}

bool isOnCycle(CgNodePtr node) {
  CgNodePtrSet visitedNodes;
  std::queue<CgNodePtr> workQueue;
  workQueue.push(node);
  while (!workQueue.empty()) {
    auto currentNode = workQueue.front();
    workQueue.pop();

    if (visitedNodes.count(currentNode) == 0) {
      visitedNodes.insert(currentNode);

      for (auto child : currentNode->getChildNodes()) {
        if (child == node) {
          return true;
        }
        workQueue.push(child);
      }
    }
  }
  return false;
}

void reachableFromMainAnalysis(CgNodePtr mainNode) {
  CgNodePtrSet visitedNodes;
  std::queue<CgNodePtr> workQueue;
  workQueue.push(mainNode);

  while (!workQueue.empty()) {
    auto node = workQueue.front();
    workQueue.pop();

    // mark as visited and reachable
    spdlog::get("console")->trace("Setting reachable: {}", node->getFunctionName());
    node->setReachable(true);
    visitedNodes.insert(node);

    // children need to be processed
    for (auto childNode : node->getChildNodes()) {
      if (visitedNodes.find(childNode) == visitedNodes.end()) {
        workQueue.push(childNode);
      }
    }
  }
}

Statements visitNodeForInclusiveStatements(CgNodePtr node, CgNodePtrSet *visitedNodes) {
  if (visitedNodes->find(node) != visitedNodes->end()) {
    return node->get<LoadImbalance::LIMetaData>()->getNumberOfInclusiveStatements();
  }
  visitedNodes->insert(node);

  CgNodePtrSet visistedChilds;

  Statements inclusiveStatements = node->get<PiraOneData>()->getNumberOfStatements();
  for (auto childNode : node->getChildNodes()) {
    // prevent double processing
    if (visistedChilds.find(childNode) != visistedChilds.end())
      continue;
    visistedChilds.insert(childNode);

    // approximate statements of a abstract function with maximum of its children (potential call targets)
    if (node->get<LoadImbalance::LIMetaData>()->isVirtual() && node->get<PiraOneData>()->getNumberOfStatements() == 0) {
      inclusiveStatements = std::max(inclusiveStatements, visitNodeForInclusiveStatements(childNode, visitedNodes));
    } else {
      inclusiveStatements += visitNodeForInclusiveStatements(childNode, visitedNodes);
    }
  }

  node->get<LoadImbalance::LIMetaData>()->setNumberOfInclusiveStatements(inclusiveStatements);

  spdlog::get("console")->trace("Visiting node " + node->getFunctionName() +
                                ". Result = " + std::to_string(inclusiveStatements));
  return inclusiveStatements;
}

void calculateInclusiveStatementCounts(CgNodePtr mainNode) {
  CgNodePtrSet visitedNodes;

  spdlog::get("console")->trace("Starting inclusive statement counting. mainNode = " + mainNode->getFunctionName());

  visitNodeForInclusiveStatements(mainNode, &visitedNodes);
}

// note: a function is reachable from itself
bool reachableFrom(CgNodePtr parentNode, CgNodePtr childNode) {
  if (parentNode == childNode) {
    return true;
  }

  // XXX RN: once again code duplication
  CgNodePtrSet visitedNodes;
  std::queue<CgNodePtr> workQueue;
  workQueue.push(parentNode);

  while (!workQueue.empty()) {
    auto node = workQueue.front();
    workQueue.pop();

    if (node == childNode) {
      return true;
    }

    visitedNodes.insert(node);

    for (auto childNode : node->getChildNodes()) {
      if (visitedNodes.find(childNode) == visitedNodes.end()) {
        workQueue.push(childNode);
      }
    }
  }

  return false;
}

CgNodePtrUnorderedSet allNodesToMain(CgNodePtr startNode, CgNodePtr mainNode,
                                     const std::unordered_map<CgNodePtr, CgNodePtrUnorderedSet> &init) {
  {
    auto it = init.find(startNode);
    if (it != init.end()) {
      return (*it).second;
    }
  }

  CgNodePtrUnorderedSet pNodes;
  pNodes.insert(mainNode);

  CgNodePtrUnorderedSet visitedNodes;
  std::queue<CgNodePtr> workQueue;
  workQueue.push(startNode);

  while (!workQueue.empty()) {
    auto node = workQueue.front();
    workQueue.pop();

    visitedNodes.insert(node);

    // XXX This is itself an expensive computation!
    //  if (reachableFrom(mainNode, node)){
    //    pNodes.insert(node);
    //  }
    if (!node->isReachable()) {
      continue;
    } else {
      pNodes.insert(node);
    }

    auto pns = node->getParentNodes();
    for (auto pNode : pns) {
      if (visitedNodes.find(pNode) == visitedNodes.end()) {
        workQueue.push(pNode);
      }
    }
  }

  return pNodes;
}

CgNodePtrUnorderedSet allNodesToMain(CgNodePtr startNode, CgNodePtr mainNode) {
  return allNodesToMain(startNode, mainNode, {});
}

/** Returns a set of all descendants including the starting node */
CgNodePtrSet getDescendants(CgNodePtr startingNode) {
  // CgNodePtrUnorderedSet childs;
  CgNodePtrSet childs;
  std::queue<CgNodePtr> workQueue;
  workQueue.push(startingNode);

  while (!workQueue.empty()) {
    auto node = workQueue.front();
    workQueue.pop();

    childs.insert(node);

    for (auto childNode : node->getChildNodes()) {
      if (childs.find(childNode) == childs.end()) {
        workQueue.push(childNode);
      }
    }
  }
  return childs;
}

/** Returns a set of all ancestors including the startingNode */
CgNodePtrSet getAncestors(CgNodePtr startingNode) {
  CgNodePtrSet ancestors;
  std::queue<CgNodePtr> workQueue;
  workQueue.push(startingNode);

  while (!workQueue.empty()) {
    auto node = workQueue.front();
    workQueue.pop();

    ancestors.insert(node);

    for (auto parentNode : node->getParentNodes()) {
      if (ancestors.find(parentNode) == ancestors.end()) {
        workQueue.push(parentNode);
      }
    }
  }

  return ancestors;
}

double calcRuntimeThreshold(const Callgraph &cg, bool useLongAsRef) {
  std::vector<double> rt;
  for (const auto &n : cg) {
    const auto &[hasBPD, bpd] = n->checkAndGet<BaseProfileData>();
    if (hasBPD) {
      spdlog::get("console")->trace("Found BaseProfileData for {}: Adding inclusive runtime of {} to RT vector.",
                                    n->getFunctionName(), bpd->getInclusiveRuntimeInSeconds());
      if (bpd->getInclusiveRuntimeInSeconds() != 0) {
        rt.push_back(bpd->getInclusiveRuntimeInSeconds());
      }
    }
  }
  spdlog::get("console")->info("The number of elements for runtime threshold calculation: {}", rt.size());

  std::sort(rt.begin(), rt.end());
  {  // raii
    std::string runtimeStr;
    for (const auto r : rt) {
      runtimeStr += ' ' + std::to_string(r);
    }
    spdlog::get("console")->debug("Runtime vector [values are seconds]: {}", runtimeStr);
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

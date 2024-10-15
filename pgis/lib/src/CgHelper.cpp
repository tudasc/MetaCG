/**
 * File: CgHelper.cpp
 * License: Part of the metacg project. Licensed under BSD 3 clause license. See LICENSE.txt file at
 * https://github.com/tudasc/metacg/LICENSE.txt
 */

#include "CgHelper.h"
#include <loadImbalance/LIMetaData.h>

#include "MetaData/PGISMetaData.h"

using namespace metacg;

namespace CgHelper {

using namespace pira;

/** returns true for nodes with two or more parents */
bool isConjunction(const metacg::CgNode* node, const metacg::Callgraph* const graph) {
  return (graph->getCallers(node).size() > 1);
}

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
    if (path.find(node) == path.end()) {
      path.insert(node);
      if (metacg::pgis::isInstrumented(node) || metacg::pgis::isInstrumentedPath(node) ||
          /*node.isRootNode()*/ graph->getCallers(node).empty()) {
        continue;
      }

      for (auto parentNode : graph->getCallers(node)) {
        workQueue.push(parentNode);
      }
    }
  }

  return {path.begin(), path.end()};
}

Statements visitNodeForInclusiveStatements(metacg::CgNode* node, CgNodeRawPtrUSet* visitedNodes,
                                           const metacg::Callgraph* const graph) {
  if (visitedNodes->find(node) != visitedNodes->end()) {
    return node->getOrCreateMD<LoadImbalance::LIMetaData>()->getNumberOfInclusiveStatements();
  }
  visitedNodes->insert(node);

  CgNodeRawPtrUSet visitedChilds;

  Statements inclusiveStatements = node->getOrCreateMD<PiraOneData>()->getNumberOfStatements();
  for (const auto& childNode : graph->getCallees(node)) {
    // prevent double processing
    if (visitedChilds.find(childNode) != visitedChilds.end()) {
      continue;
    }
    visitedChilds.insert(childNode);

    // approximate statements of a abstract function with maximum of its children (potential call targets)
    if (node->getOrCreateMD<LoadImbalance::LIMetaData>()->isVirtual() &&
        node->getOrCreateMD<PiraOneData>()->getNumberOfStatements() == 0) {
      inclusiveStatements =
          std::max(inclusiveStatements, visitNodeForInclusiveStatements(childNode, visitedNodes, graph));
    } else {
      inclusiveStatements += visitNodeForInclusiveStatements(childNode, visitedNodes, graph);
    }
  }

  node->getOrCreateMD<LoadImbalance::LIMetaData>()->setNumberOfInclusiveStatements(inclusiveStatements);

  metacg::MCGLogger::instance().getConsole()->trace("Visiting node " + node->getFunctionName() +
                                                    ". Result = " + std::to_string(inclusiveStatements));
  return inclusiveStatements;
}

void calculateInclusiveStatementCounts(metacg::CgNode* mainNode, const metacg::Callgraph* const graph) {
  CgNodeRawPtrUSet visitedNodes;

  metacg::MCGLogger::instance().getConsole()->trace("Starting inclusive statement counting. mainNode = " +
                                                    mainNode->getFunctionName());

  visitNodeForInclusiveStatements(mainNode, &visitedNodes, graph);
}

CgNodeRawPtrUSet allNodesToMain(metacg::CgNode* startNode, metacg::CgNode* mainNode,
                                const metacg::Callgraph* const graph,
                                const std::unordered_map<metacg::CgNode*, CgNodeRawPtrUSet>& init,
                                metacg::analysis::ReachabilityAnalysis& ra) {
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

    if (visitedNodes.find(node) == visitedNodes.end()) {
      visitedNodes.insert(node);

      if (ra.isReachableFromMain(node)) {
        pNodes.insert(node);
      } else {
        continue;
      }

      auto pns = graph->getCallers(node);
      for (auto pNode : pns) {
        workQueue.push(pNode);
      }
    }
  }

  return pNodes;
}

CgNodeRawPtrUSet allNodesToMain(metacg::CgNode* startNode, metacg::CgNode* mainNode,
                                const metacg::Callgraph* const graph, metacg::analysis::ReachabilityAnalysis& ra) {
  return allNodesToMain(startNode, mainNode, graph, {}, ra);
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
    if (const auto [it, inserted] = childs.insert(node); inserted) {
      for (auto childNode : graph->getCallees(node)) {
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
    if (const auto [it, inserted] = ancestors.insert(node); inserted) {
      for (auto parentNode : graph->getCallers(node)) {
        workQueue.push(parentNode);
      }
    }
  }

  return ancestors;
}

/**
 *
 * @param cg
 * @param useLongAsRef True = half max, false = median
 * @return
 */
double calcRuntimeThreshold(const Callgraph& cg, bool useLongAsRef) {
  std::vector<double> rt;
  for (const auto& elem : cg.getNodes()) {
    const auto& n = elem.second.get();
    const auto& [hasBPD, bpd] = n->checkAndGet<BaseProfileData>();
    if (hasBPD) {
      metacg::MCGLogger::instance().getConsole()->trace(
          "Found BaseProfileData for {}: Adding inclusive runtime of {} to RT vector.", n->getFunctionName(),
          bpd->getInclusiveRuntimeInSeconds());
      if (bpd->getInclusiveRuntimeInSeconds() != 0) {
        rt.push_back(bpd->getInclusiveRuntimeInSeconds());
      }
    }
  }
  metacg::MCGLogger::instance().getConsole()->info("The number of elements for runtime threshold calculation: {}",
                                                   rt.size());

  if (rt.empty()) {
    const char* errorMsg =
        "There was no function with runtime data found in profile, so we are unable to compute a runtime threshold.";
    metacg::MCGLogger::instance().getErrConsole()->error(errorMsg);
    throw std::runtime_error(errorMsg);
  }

  std::sort(rt.begin(), rt.end());
  {  // raii
    std::string runtimeStr;
    for (const auto r : rt) {
      runtimeStr += ' ' + std::to_string(r);
    }
    metacg::MCGLogger::instance().getConsole()->debug("Runtime vector [values are seconds]: {}", runtimeStr);
  }

  size_t lastIndex = rt.size() >> 1;
  if (useLongAsRef) {
    const float runtimeFilterThresholdAlpha = 0.5f;
    // FIXME: Pira is extremly sensitive about this parameter. -2 to skip the main function
    lastIndex = std::max(rt.size() - 1, (size_t)0);      // TODO: Make this adjustable
    return rt[lastIndex] * runtimeFilterThresholdAlpha;  // runtime filtering threshold
  }
  // Returns the median of the data
  return rt[lastIndex];
}

double getEstimatedCallsFromNode(metacg::Callgraph* graph, metacg::CgNode* node,
                                 const std::string& calledFunctionName) {
  const auto pCCMD = node->get<CallCountEstimationMetaData>();
  const auto IFunctionCall = pCCMD->calledFunctions.find(calledFunctionName);
  if (IFunctionCall == pCCMD->calledFunctions.end()) {
    // We don't have info about this function
    return 1;
  }
  double ret = 0;
  for (const auto& call : IFunctionCall->second) {
    // Iterate over all the different places where the function gets called and sum them up
    if (call.second.empty()) {
      // Special case. If the region is empty it gets directly called in the function or was patched in
      ret += call.first;
    } else {
      // Walk the regions
      auto regionName = call.second;
      double factor = 1.0;
      while (true) {
        const auto IRegion = pCCMD->codeRegions.find(regionName);
        assert(IRegion != pCCMD->codeRegions.end());
        std::vector<double> tempResults;
        for (const auto& rfunc : IRegion->second.functions) {
          if (rfunc != calledFunctionName) {
            const auto rnode = graph->getNode(rfunc);
            assert(rnode);
            const auto info = rnode->checkAndGet<InstrumentationResultMetaData>();
            if (info.first) {
              const auto& rfuncCalledInfo = pCCMD->calledFunctions[rfunc];
              auto calledInRegionIter = std::find_if(rfuncCalledInfo.begin(), rfuncCalledInfo.end(),
                                                     [&regionName](const auto& i) { return i.second == regionName; });
              if (calledInRegionIter == rfuncCalledInfo.end()) {
                // We did not find the region. This is either a bug or more likely the other function was patched in.
                // Search again to check if it was patched in
                calledInRegionIter = std::find_if(rfuncCalledInfo.begin(), rfuncCalledInfo.end(),
                                                  [](const auto& i) { return i.second == ""; });
              }
              assert(calledInRegionIter != rfuncCalledInfo.end());
              const auto regionCallCount = static_cast<double>(info.second->callsFromParents[node->getFunctionName()]) /
                                           calledInRegionIter->first;
              tempResults.push_back(regionCallCount);
            }
          }
        }
        if (!tempResults.empty()) {
          ret += (calculateMedianAveraged(tempResults)) * call.first * factor;
          break;  // We found our info
        }
        // Move on to the region one up
        regionName = IRegion->second.parent;
        factor *= IRegion->second.parentCalls;
        if (regionName.empty()) {
          // we ended at the function level
          ret += call.first * factor;
          break;
        }
      }
    }
  }
  return ret;
}

}  // namespace CgHelper

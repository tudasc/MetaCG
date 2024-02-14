/**
 * File: IPCGEstimatorPhase.cpp
 * License: Part of the MetaCG project. Licensed under BSD 3 clause license. See LICENSE.txt file at
 * https://github.com/tudasc/metacg/LICENSE.txt
 */

#include "ReachabilityAnalysis.h"

#include "CgHelper.h"
#include "IPCGEstimatorPhase.h"
#include "MetaData/CgNodeMetaData.h"
#include "MetaData/PGISMetaData.h"
#include "Utility.h"
#include "config/GlobalConfig.h"

#include "ParameterConfig.h"
#include "spdlog/sinks/stdout_color_sinks.h"
#include "spdlog/spdlog.h"

#include <iostream>
#include <random>
#include <unordered_map>

using namespace metacg;
using namespace pira;

FirstNLevelsEstimatorPhase::FirstNLevelsEstimatorPhase(int levels)
    : EstimatorPhase(std::string("FirstNLevels") + std::to_string(levels), nullptr), levels(levels) {}

FirstNLevelsEstimatorPhase::~FirstNLevelsEstimatorPhase() {}

void FirstNLevelsEstimatorPhase::modifyGraph(metacg::CgNode *mainMethod) { instrumentLevel(mainMethod, levels); }

void FirstNLevelsEstimatorPhase::instrumentLevel(metacg::CgNode *parentNode, int levelsLeft) {
  if (levelsLeft == 0) {
    return;
  }

  pgis::instrumentNode(parentNode);

  for (auto childNode : graph->getCallees(parentNode)) {
    instrumentLevel(childNode, levelsLeft - 1);
  }
}

//// STATEMENT COUNT ESTIMATOR PHASE

StatementCountEstimatorPhase::StatementCountEstimatorPhase(int numberOfStatementsThreshold,
                                                           metacg::Callgraph *callgraph, bool inclusiveMetric,
                                                           StatisticsEstimatorPhase *prevStatEP)
    : EstimatorPhase((inclusiveMetric ? std::string("Incl-") : std::string("Excl-")) + std::string("StatementCount-") +
                         std::to_string(numberOfStatementsThreshold),
                     callgraph),
      numberOfStatementsThreshold(numberOfStatementsThreshold),
      inclusiveMetric(inclusiveMetric),
      pSEP(prevStatEP) {}

StatementCountEstimatorPhase::~StatementCountEstimatorPhase() {}

void StatementCountEstimatorPhase::modifyGraph(metacg::CgNode *mainMethod) {
  metacg::analysis::ReachabilityAnalysis ra(graph);
  auto console = metacg::MCGLogger::instance().getConsole();

  if (pSEP) {
    numberOfStatementsThreshold = pSEP->getCuttoffNumInclStmts();
    console->debug("Changed count: now using {} as threshold", numberOfStatementsThreshold);
  }

  for (const auto &elem : graph->getNodes()) {
    const auto &node = elem.second.get();
    console->trace("Processing node: {}", node->getFunctionName());
    if (!ra.isReachableFromMain(node)) {
      console->trace("\tskipping.");
      continue;
    }
    console->trace("\testimating.");
    estimateStatementCount(node, ra);
  }
}

void StatementCountEstimatorPhase::estimateStatementCount(metacg::CgNode *startNode,
                                                          metacg::analysis::ReachabilityAnalysis &ra) {
  int inclStmtCount = 0;
  if (inclusiveMetric) {
    // INCLUSIVE
    std::queue<metacg::CgNode *> workQueue;
    workQueue.push(startNode);
    std::set<metacg::CgNode *> visitedNodes;

    while (!workQueue.empty()) {
      auto node = workQueue.front();
      const auto nodePOD = node->getOrCreateMD<PiraOneData>();
      workQueue.pop();

      if (const auto [it, inserted] = visitedNodes.insert(node); inserted) {
        inclStmtCount += nodePOD->getNumberOfStatements();

        for (auto childNode : graph->getCallees(node)) {
          if (ra.isReachableFromMain(childNode)) {
            workQueue.push(childNode);
          }
        }
      }
    }
    inclStmtCounts[startNode] = inclStmtCount;
  } else {
    // EXCLUSIVE
    const auto snPOD = startNode->getOrCreateMD<PiraOneData>();
    inclStmtCount = snPOD->getNumberOfStatements();
  }

  auto console = metacg::MCGLogger::instance().getConsole();
  console->trace("Function: {} >> InclStatementCount: {}", startNode->getFunctionName(), inclStmtCount);
  if (inclStmtCount >= numberOfStatementsThreshold) {
    console->trace("Function {} added to instrumentation list", startNode->getFunctionName());
    pgis::instrumentNode(startNode);
  }
  auto useCSInstr = pgis::config::GlobalConfig::get().getAs<bool>(pgis::options::useCallSiteInstrumentation.cliName);
  if (useCSInstr && /*!startNode->get<PiraOneData>()->getHasBody()*/ !startNode->getHasBody() &&
      startNode->get<BaseProfileData>()->getRuntimeInSeconds() == .0) {
    console->trace("Function {} added to instrumentation path", startNode->getFunctionName());
    pgis::instrumentPathNode(startNode);
  }
}

// Runtime estimator phase
RuntimeEstimatorPhase::RuntimeEstimatorPhase(metacg::Callgraph *cg, double runTimeThreshold, bool inclusiveMetric)
    : EstimatorPhase((inclusiveMetric ? std::string("Incl-") : std::string("Excl-")) + std::string("Runtime-") +
                         std::to_string(runTimeThreshold),
                     cg),
      runTimeThreshold(runTimeThreshold),
      inclusiveMetric(inclusiveMetric) {
  assert(inclusiveMetric && "Only run this metric in inclusive mode");
  const auto &gConfig = pgis::config::GlobalConfig::get();
  useCSInstrumentation = gConfig.getAs<bool>(pgis::options::useCallSiteInstrumentation.cliName);
  onlyEligibleNodes = gConfig.getAs<bool>(pgis::options::onlyInstrumentEligibleNodes.cliName);
  const int targetOverhead = gConfig.getAs<int>(pgis::options::targetOverhead.cliName);
  if (targetOverhead > 0 &&
      pgis::config::getSelectedOverheadAlgorithm() != pgis::options::OverheadSelection::OverheadSelectionEnum::None) {
    const auto &overheadConfig = pgis::config::ParameterConfig::get().getOverheadConfig();
    thresholdHotspotInclusive = overheadConfig->thresholdHotspotInclusive;
    thresholdHotspotExclusive = overheadConfig->thresholdHotspotExclusive;
    budgetForExploration = overheadConfig->percentageExplorationBudget;
    recursionFactor = overheadConfig->recursionScaleFactor;
    mpiFunctionsStatementEstimate = overheadConfig->mpiFunctionsStatementEstimate;
    inlineFactor = overheadConfig->inlineScaleFactor;
    const int prevOverhead = gConfig.getAs<int>(pgis::options::prevOverhead.cliName);
    const double targetOverheadScaled = targetOverhead - 1000;
    // Workaround to deal with negative overhead
    // TODO, maybe change the minimum overhead from 1 to something else?
    const double prevOverheadScaled = (prevOverhead > 1000) ? prevOverhead - 1000 : 1;
    assert(prevOverheadScaled > 0);
    relativeOverhead = targetOverheadScaled / prevOverheadScaled;
    if (relativeOverhead > 10) {
      // Cap the overhead to prevent massive overshooting
      relativeOverhead = 10;
    }
    metacg::MCGLogger::instance().getConsole()->info("targetOverheadScaled: {}, prevOverheadScaled: {}",
                                                     targetOverheadScaled, prevOverheadScaled);
  }
}

RuntimeEstimatorPhase::~RuntimeEstimatorPhase() {}

void RuntimeEstimatorPhase::modifyGraph(metacg::CgNode *mainMethod) {
  if (pgis::config::getSelectedOverheadAlgorithm() != pgis::options::OverheadSelection::OverheadSelectionEnum::None) {
    modifyGraphOverhead(mainMethod);
    return;
  }

  metacg::analysis::ReachabilityAnalysis ra(graph);

  for (const auto &elem : graph->getNodes()) {
    const auto &node = elem.second.get();
    estimateRuntime(node);
  }

  runTimeThreshold = CgHelper::calcRuntimeThreshold(*graph, true);
  metacg::MCGLogger::instance().getConsole()->debug("The runtime is threshold is computed as: {}", runTimeThreshold);

  // The main method is always in the dominant runtime path
  mainMethod->getOrCreateMD<PiraOneData>()->setDominantRuntime();

  std::queue<metacg::CgNode *> workQueue;
  workQueue.push(mainMethod);
  CgNodeRawPtrUSet visitedNodes;

  while (!workQueue.empty()) {
    auto node = workQueue.front();
    workQueue.pop();
    // Only visit unseen, profiled nodes. Only those have actual timing info!
    if (visitedNodes.find(node) == visitedNodes.end()) {
      visitedNodes.insert(node);
      doInstrumentation(node, ra);

      for (auto childNode : graph->getCallees(node)) {
        workQueue.push(childNode);
      }
    }
  }

  // Always instrument main
  pgis::instrumentNode(mainMethod);
}

void RuntimeEstimatorPhase::estimateRuntime(metacg::CgNode *startNode) {
  if (inclusiveMetric) {
    // INCLUSIVE
    inclRunTime[startNode] = startNode->get<BaseProfileData>()->getInclusiveRuntimeInSeconds();
    const auto calls = startNode->get<BaseProfileData>()->getNumberOfCalls();
    totalExclusiveCalls += calls;
    // Skip not leave nodes
    const auto &childs = graph->getCallees(startNode);
    if (calls > 0 && std::none_of(childs.begin(), childs.end(), [](const auto cnode) {
          return cnode->template getOrCreateMD<PiraOneData>()->comesFromCube();
        })) {
      exclusiveCalls.emplace(calls, startNode);
    }

  } else {
    // EXCLUSIVE
    assert(false && "Only run this metric in inclusive mode.");
  }
}

void RuntimeEstimatorPhase::doInstrumentation(metacg::CgNode *startNode, metacg::analysis::ReachabilityAnalysis &ra) {
  auto console = metacg::MCGLogger::instance().getConsole();
  auto runTime = inclRunTime[startNode];
  console->debug("Processing {}:\n\tNode RT:\t{}\n\tCalced RT:\t{}\n\tThreshold:\t{}", startNode->getFunctionName(),
                 startNode->get<BaseProfileData>()->getInclusiveRuntimeInSeconds(), runTime, runTimeThreshold);
  if (pgis::config::getSelectedOverheadAlgorithm() != pgis::options::OverheadSelection::OverheadSelectionEnum::None) {
    // Add new nodes according to
  } else if (runTime >= runTimeThreshold) {
    // keep the nodes on the paths in the profile, when they expose sufficient runtime.
    pgis::instrumentNode(startNode);
    console->info("Instrumenting {} because of its runtime", startNode->getFunctionName());
    int instrChildren = 0;

    std::map<metacg::CgNode *, long int> childStmts;
    long int maxStmts = 0;
    long int totStmts = 0;
    // If we have at least one child, initialize maxRtChild with the first child node
    metacg::CgNode *maxRtChild{nullptr};

    for (auto childNode : graph->getCallees(startNode)) {
      StatementCountEstimatorPhase scep(999999999, graph);  // prevent pass from instrumentation
      scep.estimateStatementCount(childNode, ra);
      childStmts[childNode] = scep.getNumStatements(childNode);

      if (childStmts[childNode] > maxStmts) {
        maxStmts = childStmts[childNode];
      }

      if (childNode->getOrCreateMD<PiraOneData>()->comesFromCube()) {
        if (childNode->get<BaseProfileData>()->getInclusiveRuntimeInSeconds() < runTimeThreshold) {
          continue;
        }
        if (maxRtChild == nullptr) {
          if (childNode == startNode) {
            continue;
          }
          maxRtChild = childNode;
        } else if (maxRtChild->getFunctionName() == childNode->getFunctionName() && maxRtChild != childNode) {
          // std::cerr << "\nWARNING NAMES EQUAL< POINTERS NOT\n";
        } else if (childNode->get<BaseProfileData>()->getInclusiveRuntimeInSeconds() >=
                   maxRtChild->get<BaseProfileData>()->getInclusiveRuntimeInSeconds()) {
          if (childNode == startNode) {
            continue;
          }
          maxRtChild = childNode;
        }
      }
      totStmts += childStmts[childNode];
    }

    if (maxRtChild) {
      console->debug("The principal node set to {}", maxRtChild->getFunctionName());
    }

    long int stmtThreshold = maxStmts;
    // XXX Original PIRA I value
    float alpha = .1f;

    /* PIRA I: simply uses the size of the children nodes container.
     * XXX: We should only consider children that have a body defines, i.e., that are eligible for instrumentation
     * with our method. This specifically excludes, for example, stdlib functions.
     */
    auto numChildren = graph->getCallees(startNode).size();
#define NEW_PIRA_ONE 1
// #undef NEW_PIRA_ONE
#ifdef NEW_PIRA_ONE
    alpha = .3f;
    numChildren = std::count_if(std::begin(graph->getCallees(startNode)), std::end(graph->getCallees(startNode)),
                                [&](const auto &n) { return childStmts[n] > 0; });
#endif
    if (numChildren > 0) {
      stmtThreshold = (maxStmts / numChildren) * alpha;
      console->debug("=== Children Info for {} ===\n\tMax Stmts:\t{}\n\tNum Children:\t{}\n\tStmt Threshold:\t{}",
                     startNode->getFunctionName(), maxStmts, numChildren, stmtThreshold);
    } else {
      stmtThreshold = 1;
    }

    console->trace(
        ">>> Algorithm Infos <<<\n\tRuntime:\t{}\n\tMax Stmts:\t{}\n\tAlpha:\t\t{}\n\tTotal Stmts:\t{}\n\tStmt "
        "Threshold:\t{}\n\tInstr Children:\t{}\n",
        runTime, maxStmts, alpha, totStmts, stmtThreshold, instrChildren);

    if (stmtThreshold < 1) {
      // This can happen, if all leaves are std lib functions.
      console->debug("Statement Threshold < 1: Returning");
      return;
    }
    if (maxRtChild) {
      console->debug("This is the dominant runtime path");
      pgis::instrumentNode(maxRtChild);
      maxRtChild->getOrCreateMD<PiraOneData>()->setDominantRuntime();
    } else {
      spdlog::get("console")->debug("This is the non-dominant runtime path");
      if (startNode->getOrCreateMD<PiraOneData>()->isDominantRuntime()) {
        spdlog::get("console")->debug("\tPrincipal: {}", startNode->getFunctionName());
        for (auto child : graph->getCallees(startNode)) {
          console->trace("\tEvaluating {} with {} [stmt threshold: {}]", child->getFunctionName(), childStmts[child],
                         stmtThreshold);
          if (childStmts[child] > stmtThreshold) {
            pgis::instrumentNode(child);
            instrChildren++;
          }
        }
      }
    }
    console->debug(
        "End of Processing {}:\n\tRuntime:\t{}\n\tMax Stmts:\t{}\n\tAlpha:\t\t{}\n\tTotal Stmt:\t{}\n\tStmt "
        "Threshold:\t{}\n\tInstr Children:\t{}",
        startNode->getFunctionName(), runTime, maxStmts, alpha, totStmts, stmtThreshold, instrChildren);
  }
}

InstumentationInfo RuntimeEstimatorPhase::getEstimatedInfoForInstrumentedNode(CgNode *node) {
  std::queue<CgNode *> workQueue;
  workQueue.push(node);
  std::set<CgNode *> visitedNodes;
  unsigned long inclusiveStmtCount = 0;
  unsigned long exclusiveStmtCount = 0;
  while (!workQueue.empty()) {
    auto wnode = workQueue.front();
    const auto nodePOD = wnode->getOrCreateMD<PiraOneData>();
    workQueue.pop();
    if (visitedNodes.find(wnode) == visitedNodes.end()) {
      visitedNodes.insert(wnode);
      // Give MPI functions that can be instrumented but don't have a body a statement count too
      // TODO JR make this configurable
      const unsigned long perNodeStmtCount = (!wnode->getHasBody() && isMPIFunction(wnode))
                                                 ? mpiFunctionsStatementEstimate
                                                 : nodePOD->getNumberOfStatements();
      inclusiveStmtCount += perNodeStmtCount;

      if (wnode == node) {
        exclusiveStmtCount = perNodeStmtCount;
      }

      for (const auto &childNode : graph->getCallees(wnode)) {
        workQueue.push(childNode);
      }
    }
  }
  double callsFromParent = getEstimatedCallCountForNode(node, exclusiveStmtCount);
  pira::InstumentationInfo ret(callsFromParent, inclusiveStmtCount, exclusiveStmtCount);
  auto [hasMD, md] = node->checkAndGet<TemporaryInstrumentationDecisionMetadata>();
  if (hasMD) {
    if (md->info.Init) {
      assert(md->info == ret && "Sanity check to ensure the instrumentation info is always the same");
    }
  } else {
    md = node->getOrCreateMD<TemporaryInstrumentationDecisionMetadata>();
    md->info = ret;
  }
  return ret;
}

void RuntimeEstimatorPhase::modifyGraphOverhead(metacg::CgNode *mainMethod) {
  // 1: Add each node to the instrumentation that was instrumented before and calculate some meta information at the
  // same time
  std::vector<CgNode *> nodesNoHotspot;
  std::vector<CgNode *> nodeInclusiveHotspot;
  std::map<CgNode *, std::vector<CgNode *>> childsToPotentialInstrumentCollection;
  std::set<CgNode *, NodeProfitComparator> childsToPotentialInstrument;

  const auto mainRuntimeInclusive = mainMethod->get<InstrumentationResultMetaData>()->inclusiveRunTimeSum;
  for (const auto &elem : graph->getNodes()) {
    const auto &node = elem.second.get();
    node->getOrCreateMD<TemporaryInstrumentationDecisionMetadata>();
    estimateRuntime(node);
    getEstimatedInfoForInstrumentedNode(node);
  }
  for (const auto &elem : graph->getNodes()) {
    const auto &node = elem.second.get();
    if (node->getOrCreateMD<PiraOneData>()->comesFromCube()) {
      if (node->getHasBody() || isMPIFunction(node) || (!onlyEligibleNodes && !useCSInstrumentation)) {
        pgis::instrumentNode(node);
      } else if (useCSInstrumentation) {
        pgis::instrumentPathNode(node);
      }
      const auto irmd = node->get<InstrumentationResultMetaData>();
      if (irmd->inclusiveRunTimeSum >= mainRuntimeInclusive * thresholdHotspotInclusive ||
          irmd->runtime >= mainRuntimeInclusive * thresholdHotspotExclusive) {
        nodeInclusiveHotspot.push_back(node);
        if (irmd->runtime >= mainRuntimeInclusive * thresholdHotspotExclusive) {
          for (const auto &C : graph->getCallees(node)) {
            C->getOrCreateMD<TemporaryInstrumentationDecisionMetadata>()->parentHasHighExclusiveRuntime = true;
          }
        }
      } else {
        nodesNoHotspot.push_back(node);
      }
    }
  }

  // 2: Kick nodes until we are under the total overhead budget and have space for exploration

  // Estimate for the total number of calls we can instrument while staying under the total overhead budget
  const double totalAllowedCalls = totalExclusiveCalls * relativeOverhead;

  // The maximum amount of calls we can keep without breaking the overhead budget and while still having space for
  // exploration
  const double callsToKeep =
      std::min(totalAllowedCalls * (1 - budgetForExploration), static_cast<double>(totalExclusiveCalls));

  // The number of calls we need to kick from the current estimation to make enough space
  const double callsToKick = totalExclusiveCalls - callsToKeep >= 1 ? totalExclusiveCalls - callsToKeep : 0;
  assert(totalAllowedCalls > 0);

  metacg::MCGLogger::instance().getConsole()->info(
      "Total calls: {} relative overhead: {} allowed calls: {} calls to kick: {} calls to keep: {}",
      totalExclusiveCalls, relativeOverhead, totalAllowedCalls, callsToKick, callsToKeep);

  // Idea: Kick nodes based on their runtime(sum) per call  (nodes with a low runtime are less attractive) until we have
  // kicked enough calls;

  // calls kicked from the instrumentation while lowering the overhead or making space for more exploration
  double kicked = 0;
  if (callsToKick > 0) {
    switch (pgis::config::getSelectedOverheadAlgorithm()) {
      case pgis::options::OverheadSelection::OverheadSelectionEnum::None:
        assert(false && "This should be unreachable");
        break;
      case pgis::options::OverheadSelection::OverheadSelectionEnum::Random:
        kicked = kickNodesRandomly(mainMethod, nodesNoHotspot, nodeInclusiveHotspot, callsToKick);
        break;
      case pgis::options::OverheadSelection::OverheadSelectionEnum::TimePerCall:
        kicked = kickNodesByRuntimePerCall(mainMethod, nodesNoHotspot, nodeInclusiveHotspot, callsToKick);
        break;
      case pgis::options::OverheadSelection::OverheadSelectionEnum::TimePerCallKeepSmall:
        kicked = kickNodesByRuntimePerCallKeepSmall(mainMethod, nodesNoHotspot, nodeInclusiveHotspot, callsToKick);
        break;
    }
  }
  for (const auto &elem : graph->getNodes()) {
    const auto &node = elem.second.get();
    if (pgis::isAnyInstrumented(node)) {
      metacg::MCGLogger::instance().getConsole()->debug("Node left after kicking: {}", node->getFunctionName());
    }
  }

  // Find direct childs nodes to potentially instrument
  for (const auto &elem : graph->getNodes()) {
    const auto &node = elem.second.get();
    if (node->getOrCreateMD<PiraOneData>()->comesFromCube()) {
      // TODO: Do we need this check
      if (!node->get<TemporaryInstrumentationDecisionMetadata>()->isKicked) {
        for (const auto &C : graph->getCallees(node)) {
          if (!pgis::isAnyInstrumented(C)) {
            childsToPotentialInstrumentCollection[C].push_back(node);
          }
        }
      }
    }
  }

  const auto filterEligible = [&childsToPotentialInstrumentCollection, &childsToPotentialInstrument, this]() {
    for (const auto &C : childsToPotentialInstrumentCollection) {
      if (const auto metaDataInstResult = C.first->checkAndGet<InstrumentationResultMetaData>();
          !metaDataInstResult.first || metaDataInstResult.second->callCount != 0 ||
          !metaDataInstResult.second->isExclusiveRuntime) {
        if (const auto metaDataInstDecision = C.first->get<TemporaryInstrumentationDecisionMetadata>();
            metaDataInstDecision->info.callsFromParents != 0) {
          if (C.first->getHasBody() || isMPIFunction(C.first) || (!onlyEligibleNodes && !useCSInstrumentation)) {
            childsToPotentialInstrument.insert(C.first);
          } else if (useCSInstrumentation) {
            if (onlyEligibleNodes) {
              if (isEligibleForPathInstrumentation(C.first, graph, C.second)) {
                childsToPotentialInstrument.insert(C.first);
              }
            } else {
              childsToPotentialInstrument.insert(C.first);
            }
          }
        }
      }
    }
    childsToPotentialInstrumentCollection.clear();
  };

  filterEligible();

  // 3: Add new nodes to the instrumentation
  // Initial available budget to instrument new nodes
  const double initialAvailableBudget = (totalAllowedCalls - (totalExclusiveCalls - kicked));
  // 100 - (200 -150) = 50
  // 400 - (200 - 0)  = 200
  // 800 - (200  - 0) = 600
  double usedBudget = 0;
  bool addedNewNodeToInstrumentation = true;  // Initial true so we always run the first time

  while (usedBudget <= initialAvailableBudget && addedNewNodeToInstrumentation) {
    const auto childsToInstrument =
        getNodesToInstrumentGreedyKnapsackOverhead(childsToPotentialInstrument, initialAvailableBudget - usedBudget);
    addedNewNodeToInstrumentation = !childsToInstrument.first.empty();
    usedBudget += childsToInstrument.second;
    for (const auto nti : childsToInstrument.first) {
      // filterEligible checked before that we only have functions that we care about
      if (nti->getHasBody() || isMPIFunction(nti) || (!onlyEligibleNodes && !useCSInstrumentation)) {
        pgis::instrumentNode(nti);
      } else {
        pgis::instrumentPathNode(nti);
      }
      for (const auto &C : graph->getCallees(nti)) {
        if (!pgis::isAnyInstrumented(C)) {
          childsToPotentialInstrumentCollection[C].push_back(nti);
        }
      }
    }
    filterEligible();
    for (const auto nti : childsToInstrument.first) {
      childsToPotentialInstrument.erase(nti);
    }
  }

  metacg::MCGLogger::instance().getConsole()->info(
      "Total allowed calls: {}, prevUsedCalls: {}, kicked: {}, usedBudget: {}", totalAllowedCalls, totalExclusiveCalls,
      kicked, usedBudget);
  // Cleanup:
  // always instrument main
  pgis::instrumentNode(mainMethod);
}

double RuntimeEstimatorPhase::kickNodesByRuntimePerCall(const CgNode *mainMethod,
                                                        std::vector<CgNode *> &nodesSortedByRuntimePerCallNoHotspot,
                                                        std::vector<CgNode *> &nodesSortedByRuntimePerCallHotspot,
                                                        const double callsToKick) const {
  double kicked = 0.0;
  if (callsToKick > 0) {
    // First sort the nodes
    std::sort(nodesSortedByRuntimePerCallNoHotspot.begin(), nodesSortedByRuntimePerCallNoHotspot.end(),
              [](const CgNode *lhs, const CgNode *rhs) {
                return lhs->get<InstrumentationResultMetaData>()->inclusiveTimePerCallSum <
                       rhs->get<InstrumentationResultMetaData>()->inclusiveTimePerCallSum;
              });
    std::sort(nodesSortedByRuntimePerCallHotspot.begin(), nodesSortedByRuntimePerCallHotspot.end(),
              [](const CgNode *lhs, const CgNode *rhs) {
                return lhs->get<InstrumentationResultMetaData>()->inclusiveTimePerCallSum <
                       rhs->get<InstrumentationResultMetaData>()->inclusiveTimePerCallSum;
              });

    kickNodesFromInstrumentation(mainMethod, nodesSortedByRuntimePerCallNoHotspot, callsToKick, kicked);
    if (kicked < callsToKick) {
      metacg::MCGLogger::instance().getConsole()->warn(
          "Could not kick enough non-hotspot nodes from the instrumentation. Starting kicking of potential "
          "hotspots...");
      kickNodesFromInstrumentation(mainMethod, nodesSortedByRuntimePerCallHotspot, callsToKick, kicked);
      if (kicked < callsToKick || mainMethod->get<TemporaryInstrumentationDecisionMetadata>()->isKicked) {
        metacg::MCGLogger::instance().getConsole()->error(
            "Could not achieve overhead goal even after kicking all nodes");
        exit(-1);
      }
    }
  }
  return kicked;
}

void RuntimeEstimatorPhase::kickNodesFromInstrumentation(const CgNode *mainMethod,
                                                         const std::vector<CgNode *> &nodesToKick,
                                                         const double callsToKick, double &kicked) const {
  for (const auto &node : nodesToKick) {
    // This could cause holes if there are cycles in the program. We fill them later, so it should not really be a
    // problem
    kickSingleNode(node, kicked);
    if (kicked >= callsToKick || node == mainMethod) {
      break;
    }
  }
}

std::pair<std::vector<CgNode *>, double> RuntimeEstimatorPhase::getNodesToInstrumentGreedyKnapsackOverhead(
    const std::set<CgNode *, NodeProfitComparator> &nodes, double costLimit) {
  auto console = metacg::MCGLogger::instance().getConsole();
  {
    // Debug code
    console->debug("Budget: {}", costLimit);
    struct CallEstimator {
      bool operator()(const metacg::CgNode *lhs, const metacg::CgNode *rhs) const {
        const auto lhsCalls = getCallsToNode(lhs);
        const auto rhsCalls = getCallsToNode(rhs);
        return std::tie(lhsCalls, lhs) > std::tie(rhsCalls, rhs);
      }
    };
    std::set<CgNode *, CallEstimator> callEstimatorSet;
    for (const auto &node : nodes) {
      callEstimatorSet.insert(node);
    }
    for (const auto &call : callEstimatorSet) {
      console->debug("Call estimate for node {}: {}", call->getFunctionName(), getCallsToNode(call));
    }
    // End debug code
  }

  std::vector<CgNode *> ret;
  double curCost = 0;
  double maxCost = 0;
  CgNode *maxProfitNode = nullptr;
  for (const auto I : nodes) {
    assert(!pgis::isAnyInstrumented(I));
    const auto cost = getCallsToNode(I);
    if (cost < costLimit) {
      if (curCost + cost <= costLimit) {
        curCost += cost;
        ret.push_back(I);
      }
      if (!maxProfitNode || NodeValueComparator::compare(I, maxProfitNode)) {
        maxProfitNode = I;
        maxCost = cost;
      }
    }
  }
  if (maxProfitNode && std::find(ret.begin(), ret.end(), maxProfitNode) == ret.end()) {
    ret.clear();
    ret.push_back(maxProfitNode);
    console->debug("Adding node {}", maxProfitNode->getFunctionName());
    return {ret, maxCost};
  }

  for (const auto node : ret) {
    console->debug("Adding node {}", node->getFunctionName());
  }
  return {ret, curCost};
}

double RuntimeEstimatorPhase::getEstimatedCallCountForNode(CgNode *node, std::set<CgNode *> &blacklist) {
  // Quick exit if we have the info already
  const auto info = node->checkAndGet<InstrumentationResultMetaData>();
  if (info.first) {
    // TODO JR: Scale this for Inline reasons ?
    return info.second->callCount;
  }

  blacklist.insert(node);
  const auto &parent = graph->getCallers(node);
  double ret = 0.0;
  double selfCalls = 0.0;
  bool selfRecursive = RuntimeEstimatorPhase::isSelfRecursive(node, graph);
  for (const auto parentNode : parent) {
    // Recursion in nodes that are not direct children
    if (blacklist.find(parentNode) != blacklist.end() && parentNode != node) {
      continue;
    } else if (parentNode == node) {
      // Direct recursion
      const auto pcc = CgHelper::getEstimatedCallsFromNode(graph, parentNode, node->getFunctionName());
      selfCalls += pcc;
    } else {
      const auto pcc = CgHelper::getEstimatedCallsFromNode(graph, parentNode, node->getFunctionName());
      const auto parentCallCount = getEstimatedCallCountForNode(parentNode, blacklist);
      ret += parentCallCount * pcc;
    }
  }
  blacklist.erase(node);
  if (selfRecursive) {
    // TODO JR make this adjustable
    metacg::MCGLogger::instance().getConsole()->debug("Applying recursion factor of {} for node {}", recursionFactor,
                                                      node->getFunctionName());
    ret *= selfCalls * recursionFactor;
  }
  return ret;
}
double RuntimeEstimatorPhase::kickNodesRandomly(const metacg::CgNode *mainMethod,
                                                std::vector<metacg::CgNode *> &nodesNoHotspot,
                                                std::vector<metacg::CgNode *> &nodesHotspot,
                                                const double callsToKick) const {
  // Set up a random number generator
  double kicked = 0.0;
  if (callsToKick > 0) {
    const int seed = 1234;       // Fixed seed for reproducibility
    std::minstd_rand rng(seed);  // C++ rngs are all rather suboptimal
    std::shuffle(nodesNoHotspot.begin(), nodesNoHotspot.end(), rng);
    std::shuffle(nodesHotspot.begin(), nodesHotspot.end(), rng);
    std::vector<metacg::CgNode *>::iterator nodeIt;
    // First try the not hotspot nodes, then the hotspot ones
    auto kickCheck = [this](metacg::CgNode *node) {
      return (node != graph->getMain()) && isLeafInstrumentationNode(node, this->graph);
    };
    auto kickCheckWeak = [this](metacg::CgNode *node) {
      return (node != graph->getMain()) && pgis::isAnyInstrumented(node);
    };
    do {
      nodeIt = std::find_if(nodesNoHotspot.begin(), nodesNoHotspot.end(), kickCheck);
      if (nodeIt == nodesNoHotspot.end()) {
        break;
      }
      kickSingleNode(*nodeIt, kicked);
      if (*nodeIt == mainMethod) {
        break;
      }

    } while (kicked < callsToKick);
    if (kicked < callsToKick) {
      metacg::MCGLogger::instance().getConsole()->warn(
          "Could not kick enough non-hotspot nodes from the instrumentation. Starting kicking of potential "
          "hotspots and nodes that may leave holes...");
      do {
        nodeIt = std::find_if(nodesNoHotspot.begin(), nodesNoHotspot.end(), kickCheckWeak);
        if (nodeIt == nodesNoHotspot.end()) {
          nodeIt = std::find_if(nodesHotspot.begin(), nodesHotspot.end(), kickCheck);
          if (nodeIt == nodesHotspot.end()) {
            nodeIt = std::find_if(nodesHotspot.begin(), nodesHotspot.end(), kickCheckWeak);
            if (nodeIt == nodesHotspot.end()) {
              break;
            }
          }
        }
        kickSingleNode(*nodeIt, kicked);
        if (*nodeIt == mainMethod) {
          break;
        }
      } while (kicked < callsToKick);
      if (kicked < callsToKick || mainMethod->get<TemporaryInstrumentationDecisionMetadata>()->isKicked) {
        metacg::MCGLogger::instance().getErrConsole()->error(
            "Could not achieve overhead goal even after kicking all nodes");
        exit(-1);
      }
    }
  }
  return kicked;
}
double RuntimeEstimatorPhase::kickNodesByRuntimePerCallKeepSmall(
    const metacg::CgNode *mainMethod, std::vector<metacg::CgNode *> &nodesSortedByRuntimePerCallNoHotspot,
    std::vector<metacg::CgNode *> &nodesSortedByRuntimePerCallHotspot, const double callsToKick) const {
  double kicked = 0.0;
  if (callsToKick > 0) {
    // First sort the node according to the calls in them
    std::sort(nodesSortedByRuntimePerCallNoHotspot.begin(), nodesSortedByRuntimePerCallNoHotspot.end(),
              [](const CgNode *lhs, const CgNode *rhs) {
                return lhs->get<InstrumentationResultMetaData>()->callCount >
                       rhs->get<InstrumentationResultMetaData>()->callCount;
              });
    // And move the 20% with the fewest calls in an extra container
    const std::size_t move_count = nodesSortedByRuntimePerCallNoHotspot.size() * 0.2;
    std::vector<metacg::CgNode *> littleCalls;
    std::move(nodesSortedByRuntimePerCallNoHotspot.end() - move_count, nodesSortedByRuntimePerCallNoHotspot.end(),
              std::back_inserter(littleCalls));
    nodesSortedByRuntimePerCallNoHotspot.erase(nodesSortedByRuntimePerCallNoHotspot.end() - move_count,
                                               nodesSortedByRuntimePerCallNoHotspot.end());
    // Sort what we have leftover
    const auto comparer = [](const CgNode *lhs, const CgNode *rhs) {
      return lhs->get<InstrumentationResultMetaData>()->inclusiveTimePerCallSum <
             rhs->get<InstrumentationResultMetaData>()->inclusiveTimePerCallSum;
    };
    std::sort(nodesSortedByRuntimePerCallNoHotspot.begin(), nodesSortedByRuntimePerCallNoHotspot.end(), comparer);
    // And start kicking:
    for (const auto &node : nodesSortedByRuntimePerCallNoHotspot) {
      // Check that we can safely kick the node
      if (isLeafInstrumentationNode(node, graph)) {
        kickSingleNode(node, kicked);
      }
      if (kicked >= callsToKick || node == mainMethod) {
        break;
      }
    }
    if (kicked < callsToKick) {
      metacg::MCGLogger::instance().getConsole()->warn(
          "Could not kick enough  nodes from the instrumentation. Starting kicking of reserved nodes...");
      // Put the saved nodes back
      nodesSortedByRuntimePerCallNoHotspot.insert(nodesSortedByRuntimePerCallNoHotspot.end(), littleCalls.begin(),
                                                  littleCalls.end());
      std::sort(nodesSortedByRuntimePerCallNoHotspot.begin(), nodesSortedByRuntimePerCallNoHotspot.end(), comparer);
      for (const auto &node : nodesSortedByRuntimePerCallNoHotspot) {
        if (pgis::isAnyInstrumented(node)) {
          kickSingleNode(node, kicked);
        }
        if (kicked >= callsToKick || node == mainMethod) {
          break;
        }
      }
      if (kicked < callsToKick) {
        metacg::MCGLogger::instance().getConsole()->warn(
            "Could not kick enough non-hotspot nodes from the instrumentation. Starting kicking of potential "
            "hotspots...");
        std::sort(nodesSortedByRuntimePerCallHotspot.begin(), nodesSortedByRuntimePerCallHotspot.end(), comparer);
        kickNodesFromInstrumentation(mainMethod, nodesSortedByRuntimePerCallHotspot, callsToKick, kicked);
        if (kicked < callsToKick || mainMethod->get<TemporaryInstrumentationDecisionMetadata>()->isKicked) {
          metacg::MCGLogger::instance().getErrConsole()->error(
              "Could not achieve overhead goal even after kicking all nodes");
          exit(-1);
        }
      }
    }
  }
  return kicked;
}
bool RuntimeEstimatorPhase::isLikelyToBeInlined(const CgNode *node, unsigned long stmtCount) {
  const auto md = node->get<pira::InlineMetaData>();
  assert(md);
  if (!node->getHasBody()) {
    return false;
  }
  if (md->markedAlwaysInline) {
    return true;
  }
  // These thresholds are based on llvms inliner
  const int inlineThreshold = 100;      // InlineSizeAllowance
  const int inlineHintThreshold = 325;  // Inlinehint-threshold
  const int instructionCost = 5;        // inline-instr-cost
  const auto stmtCountScaled = stmtCount * instructionCost;
  if (md->markedInline && stmtCountScaled <= inlineHintThreshold) {
    return true;
  }
  if ((md->likelyInline || md->isTemplate) && stmtCountScaled <= inlineThreshold) {
    return true;
  }
  return false;
}

bool RuntimeEstimatorPhase::isSelfRecursive(metacg::CgNode *node, metacg::Callgraph *cg) {
  auto callees = cg->getCallees(node);
  if (callees.find(node) != callees.end()) {
    return true;
  }
  for (const auto &child : callees) {
    auto calleeCallees = cg->getCallees(child);
    if (calleeCallees.find(node) != calleeCallees.end()) {
      return true;
    }
  }
  return false;
}
void RuntimeEstimatorPhase::kickSingleNode(metacg::CgNode *node, double &kicked) const {
  pgis::resetInstrumentation(node);
  metacg::MCGLogger::instance().getConsole()->debug("Kicking node {} with {} calls", node->getFunctionName(),
                                                    node->get<InstrumentationResultMetaData>()->callCount);
  kicked += node->get<InstrumentationResultMetaData>()->callCount;
  node->get<TemporaryInstrumentationDecisionMetadata>()->isKicked = true;
}

void StatisticsEstimatorPhase::modifyGraph(metacg::CgNode *mainMethod) {
  numFunctions = graph->getNodes().size();
  // Threshold irrelevant, only building incl aggregation of interest
  StatementCountEstimatorPhase sce(999999999, graph);
  sce.modifyGraph(mainMethod);

  const auto heuristicMode = pgis::config::getSelectedHeuristic();
  switch (heuristicMode) {
    case pgis::options::HeuristicSelection::HeuristicSelectionEnum::STATEMENTS:
      // Statements get always collected
      break;
    case pgis::options::HeuristicSelection::HeuristicSelectionEnum::CONDITIONALBRANCHES: {
      ConditionalBranchesEstimatorPhase cbe(ConditionalBranchesEstimatorPhase::limitThreshold, graph);
      cbe.modifyGraph(mainMethod);
    } break;
    case pgis::options::HeuristicSelection::HeuristicSelectionEnum::CONDITIONALBRANCHES_REVERSE: {
      ConditionalBranchesReverseEstimatorPhase cbre(ConditionalBranchesReverseEstimatorPhase::limitThreshold, graph);
      cbre.modifyGraph(mainMethod);
    } break;
    case pgis::options::HeuristicSelection::HeuristicSelectionEnum::FP_MEM_OPS: {
      FPAndMemOpsEstimatorPhase re(FPAndMemOpsEstimatorPhase::limitThreshold, graph);
      re.modifyGraph(mainMethod);
    } break;
    case pgis::options::HeuristicSelection::HeuristicSelectionEnum::LOOPDEPTH: {
      LoopDepthEstimatorPhase lde(LoopDepthEstimatorPhase::limitThreshold, graph);
      lde.modifyGraph(mainMethod);
    } break;
    case pgis::options::HeuristicSelection::HeuristicSelectionEnum::GlOBAL_LOOPDEPTH: {
      GlobalLoopDepthEstimatorPhase glde(GlobalLoopDepthEstimatorPhase::limitThreshold, graph);
      glde.modifyGraph(mainMethod);
    } break;
  }
  metacg::MCGLogger::instance().getConsole()->info("Running StatisticsEstimatorPhase::modifyGraph");

  metacg::analysis::ReachabilityAnalysis ra(graph);
  for (const auto &elem : graph->getNodes()) {
    const auto &node = elem.second.get();
    if (!ra.isReachableFromMain(node)) {
      metacg::MCGLogger::instance().getConsole()->trace("Running on non-reachable function {}",
                                                        node->getFunctionName());
      continue;
    }

    numReachableFunctions++;
    auto numStmts = node->getOrCreateMD<PiraOneData>()->getNumberOfStatements();
    if (pgis::isInstrumented(node)) {
      stmtsCoveredWithInstr += numStmts;
    }
    if (node->getOrCreateMD<PiraOneData>()->comesFromCube()) {
      stmtsActuallyCovered += numStmts;
    }
    auto &histElem = stmtHist[numStmts];
    histElem++;
    totalStmts += numStmts;
    auto &inclHistElem = stmtInclHist[sce.getNumStatements(node)];
    inclHistElem++;

    if (node->has<CodeStatisticsMetaData>()) {
      const auto csMD = node->get<CodeStatisticsMetaData>();
      totalVarDecls += csMD->numVars;
    } else {
      metacg::MCGLogger::instance().getConsole()->warn("Node does not have CodeStatisticsMetaData");
    }
  }
}

void StatisticsEstimatorPhase::printReport() {
  if (!shouldPrintReport) {
    metacg::MCGLogger::instance().getConsole()->trace("Should not print report");
    return;
  }

  const auto toRandAccContainer = [&](decltype(stmtHist) &inCont) {
    std::vector<std::remove_reference<decltype(inCont)>::type::key_type> keys;
    for (const auto &n : inCont) {
      keys.push_back(n.first);
    }
    return keys;
  };

  auto singleStmts = toRandAccContainer(stmtHist);
  std::nth_element(singleStmts.begin(), singleStmts.begin() + singleStmts.size() / 2, singleStmts.end());
  long int medianNumSingleStmts = singleStmts[singleStmts.size() / 2];

  auto inclStmts = toRandAccContainer(stmtInclHist);
  std::nth_element(inclStmts.begin(), inclStmts.begin() + inclStmts.size() / 2, inclStmts.end());

  long int medianNumStmts = inclStmts[inclStmts.size() / 2];

  using HP = decltype(stmtHist)::value_type;
  auto minMax = std::minmax_element(stmtHist.begin(), stmtHist.end(), [](HP p1, HP p2) { return p1.first < p2.first; });
  const long int maxNumSingleStmts = (*(minMax.second)).first;
  const long int minNumSingleStmts = (*(minMax.first)).first;

  auto minMaxIncl =
      std::minmax_element(stmtInclHist.begin(), stmtInclHist.end(), [](HP p1, HP p2) { return p1.first < p2.first; });
  const long int maxNumStmts = (*(minMaxIncl.second)).first;
  const long int minNumStmts = (*(minMaxIncl.first)).first;

  auto console = metacg::MCGLogger::instance().getConsole();
  console->info(
      " === Call graph statistics ===\n"
      "No. of Functions:\t{}\n"
      "No. of reach. Funcs:\t{}\n"
      "No. of statements:\t{}\n"
      "Max No. Incl Stmt:\t{}\n"
      "Median No. Incl Stmt:\t{}\n"
      "Min No. Incl Stmt:\t{}\n"
      "Max No. Stmt:\t{}\n"
      "Median No. Stmt:\t{}\n"
      "Min No. Stmt:\t{}\n"
      "Covered w/ Instr:\t{}\n"
      "Stmt not Covered:\t{}\n"
      "Median No. ConditionalBranches:\t{}\n"
      "Median Roofline:\t{}\n"
      "Median No. LoopDepth:\t{}\n"
      "Median No. GlobalLoopDepth:\t{}\n"
      "\n-------\n"
      "Total Var Decls\t{}\n"
      "\n"
      " === === === === ===",
      numFunctions, numReachableFunctions, totalStmts, maxNumStmts, medianNumStmts, minNumStmts, maxNumSingleStmts,
      medianNumSingleStmts, minNumSingleStmts, stmtsCoveredWithInstr, stmtsActuallyCovered,
      (totalStmts - stmtsCoveredWithInstr), getCuttoffConditionalBranches(), getCuttoffRoofline(),
      getCuttoffLoopDepth(), getCuttoffGlobalLoopDepth(), totalVarDecls);
  console->info(printHist(stmtInclHist, "statements"));
  console->info(printHist(conditionalBranchesInclHist, "conditionalBranches"));
  console->info(printHist(reverseConditionalBranchesInclHist, "reverseConditionalBranches"));
  console->info(printHist(rooflineInclHist, "roofline"));
  console->info(printHist(loopDepthInclHist, "loopDepth"));
  console->info(printHist(globalLoopDepthInclHist, "globalLoopDepth"));
}

std::string StatisticsEstimatorPhase::printHist(const MapT &hist, const std::string &name) {
  std::string out;
  out += "Histogram for " + name + ":\n";
  for (const auto &entry : hist) {
    out += std::to_string(entry.first) + +" : " + std::to_string(entry.second) + "\n";
  }
  if (hist.empty()) {
    return out;  // Fast exit for empty histogram
  }
  out += "Metric: Max: " + std::to_string(getHalfMaxFromHist(hist)) + "\n";
  out += "Metric: Median: " + std::to_string(getMedianFromHist(hist)) + "\n";
  out += "Metric: Unique Median: " + std::to_string(getUniqueMedianFromHist(hist)) + "\n";
  return out;
}

long int StatisticsEstimatorPhase::getCuttoffNumInclStmts() { return getCuttoffValue(stmtInclHist); }

long int StatisticsEstimatorPhase::getCuttoffValue(const MapT &hist) const {
  if (hist.empty()) {
    return 0;  // Fast exit if the map is empty. Required to prevent errors when printing an empty histogram
  }
  const auto &gConfig = pgis::config::GlobalConfig::get();
  const auto cuttoffMode = gConfig.getAs<pgis::options::CuttoffSelection>(pgis::options::cuttoffSelection.cliName).mode;
  switch (cuttoffMode) {
    case pgis::options::CuttoffSelection::CuttoffSelectionEnum::MAX:
      return getHalfMaxFromHist(hist);
    case pgis::options::CuttoffSelection::CuttoffSelectionEnum::MEDIAN:
      return getMedianFromHist(hist);
    case pgis::options::CuttoffSelection::CuttoffSelectionEnum::UNIQUE_MEDIAN:
      return getUniqueMedianFromHist(hist);
    default:
      // Should never happen
      exit(-1);
  }
}
long int StatisticsEstimatorPhase::getUniqueMedianFromHist(const MapT &hist) const {
  const auto toRandAccContainer = [&](const MapT &inCont) {
    std::vector<MapT::key_type> keys;
    for (const auto &n : inCont) {
      keys.push_back(n.first);
    }
    return keys;
  };

  auto inclStmts = toRandAccContainer(hist);
  std::nth_element(inclStmts.begin(), inclStmts.begin() + inclStmts.size() / 2, inclStmts.end());

  const long int medianNumStmts = inclStmts[inclStmts.size() / 2];
  return medianNumStmts;
}

long int StatisticsEstimatorPhase::getMedianFromHist(const MapT &hist) const {
  const auto toRandAccContainer = [&](const MapT &inCont) {
    std::vector<MapT::key_type> keys;
    for (const auto &n : inCont) {
      for (int i = 0; i < n.second; i++) {
        keys.push_back(n.first);
      }
    }
    return keys;
  };
  auto inclStmts = toRandAccContainer(hist);
  std::nth_element(inclStmts.begin(), inclStmts.begin() + inclStmts.size() / 2, inclStmts.end());

  const long int medianNumStmts = inclStmts[inclStmts.size() / 2];
  return medianNumStmts;
}

long int StatisticsEstimatorPhase::getHalfMaxFromHist(const MapT &hist) const {
  const auto toRandAccContainer = [&](const MapT &inCont) {
    std::vector<MapT::key_type> keys;
    for (const auto &n : inCont) {
      for (int i = 0; i < n.second; i++) {
        keys.push_back(n.first);
      }
    }
    return keys;
  };
  auto inclStmts = toRandAccContainer(hist);
  return (*std::max_element(inclStmts.begin(), inclStmts.end())) * 0.5;
}

long int StatisticsEstimatorPhase::getCuttoffConditionalBranches() const {
  return getCuttoffValue(conditionalBranchesInclHist);
}
long int StatisticsEstimatorPhase::getCuttoffLoopDepth() const { return getCuttoffValue(loopDepthInclHist); }
long int StatisticsEstimatorPhase::getCuttoffGlobalLoopDepth() const {
  return getCuttoffValue(globalLoopDepthInclHist);
}
long int StatisticsEstimatorPhase::getCuttoffRoofline() const { return getCuttoffValue(rooflineInclHist); }
long int StatisticsEstimatorPhase::getCuttoffReversesConditionalBranches() const {
  return getCuttoffValue(reverseConditionalBranchesInclHist);
}

// WL CALLPATH DIFFERENTIATION ESTIMATOR PHASE

WLCallpathDifferentiationEstimatorPhase::WLCallpathDifferentiationEstimatorPhase(std::string whiteListName)
    : EstimatorPhase("WLCallpathDifferentiation", nullptr), whitelistName(whiteListName) {}

WLCallpathDifferentiationEstimatorPhase::~WLCallpathDifferentiationEstimatorPhase() {}

void WLCallpathDifferentiationEstimatorPhase::modifyGraph(metacg::CgNode *mainMethod) {
  // TODO: move this parsing somewhere else
  std::ifstream file(whitelistName);
  if (!file) {
    metacg::MCGLogger::instance().getErrConsole()->error("Error in WLCallpathDifferentitation: Could not open {}",
                                                         whitelistName);
    exit(1);
  }
  std::string line;
  while (std::getline(file, line)) {
    if (line.empty()) {
      continue;
    }
    metacg::CgNode *node = graph->getNode(line);
    if (node == nullptr) {
      continue;
    }
    addNodeAndParentsToWhitelist(node);
  }
  file.close();

  for (const auto &elem : graph->getNodes()) {
    const auto &node = elem.second.get();
    if (CgHelper::isConjunction(node, graph) && (whitelist.find(node) != whitelist.end())) {
      for (const auto &parentNode : graph->getCallers(node)) {
        pgis::instrumentNode(parentNode);
      }
    }
  }
}

void WLCallpathDifferentiationEstimatorPhase::addNodeAndParentsToWhitelist(metacg::CgNode *node) {
  if (whitelist.find(node) == whitelist.end()) {
    whitelist.insert(node);

    for (const auto &parentNode : graph->getCallers(node)) {
      addNodeAndParentsToWhitelist(parentNode);
    }
  }
}

SummingCountPhaseBase::~SummingCountPhaseBase() = default;

void SummingCountPhaseBase::modifyGraph(metacg::CgNode *mainMethod) {
  metacg::analysis::ReachabilityAnalysis ra(graph);
  auto console = metacg::MCGLogger::instance().getConsole();

  if (pSEP) {
    threshold = getPreviousThreshold();
    console->debug("Changed count: now using {} as threshold", threshold);
  }
  runInitialization();
  for (const auto &elem : graph->getNodes()) {
    const auto &node = elem.second.get();
    console->trace("Processing node: {}", node->getFunctionName());
    if (!ra.isReachableFromMain(node)) {
      console->trace("\tskipping.");
      continue;
    }
    console->trace("\testimating.");
    estimateCount(node, ra);
  }
}
void SummingCountPhaseBase::estimateCount(CgNode *startNode, metacg::analysis::ReachabilityAnalysis &ra) {
  long int count = 0;
  // INCLUSIVE
  if (inclusive) {
    std::queue<metacg::CgNode *> workQueue;
    workQueue.push(startNode);
    std::set<metacg::CgNode *> visitedNodes;

    while (!workQueue.empty()) {
      auto node = workQueue.front();
      workQueue.pop();

      if (visitedNodes.find(node) == visitedNodes.end()) {
        visitedNodes.insert(node);
        count += getTargetCount(node);

        for (const auto &childNode : graph->getCallees(node)) {
          if (ra.isReachableFromMain(childNode)) {
            workQueue.push(childNode);
          }
        }
      }
    }
  } else {
    count += getTargetCount(startNode);
  }
  counts[startNode] = count;

  metacg::MCGLogger::instance().getConsole()->trace("Function: {} >> InclStatementCount: {}",
                                                    startNode->getFunctionName(), count);
  if (count >= threshold) {
    pgis::instrumentNode(startNode);
  }
  if (/*!startNode->get<PiraOneData>()->getHasBody()*/ !startNode->getHasBody() &&
      startNode->get<BaseProfileData>()->getRuntimeInSeconds() == .0) {
    // TODO JR only if cs option is set
    pgis::instrumentPathNode(startNode);
  }
}
SummingCountPhaseBase::SummingCountPhaseBase(long int threshold, const std::string &name, metacg::Callgraph *callgraph,
                                             StatisticsEstimatorPhase *prevStatEP, bool inclusive)
    : EstimatorPhase((inclusive ? "Incl-" : "Excl-") + name + "-" + std::to_string(threshold), callgraph),
      threshold(threshold),
      pSEP(prevStatEP),
      inclusive(inclusive) {}
long int SummingCountPhaseBase::getCounted(const metacg::CgNode *node) { return counts[node]; }
void SummingCountPhaseBase::runInitialization() {}

long int ConditionalBranchesEstimatorPhase::getPreviousThreshold() const {
  return pSEP->getCuttoffConditionalBranches();
}
long int ConditionalBranchesEstimatorPhase::getTargetCount(const CgNode *node) const {
  if (node->has<NumConditionalBranchMetaData>()) {
    const auto md = node->get<NumConditionalBranchMetaData>();
    return md->numConditionalBranches;
  } else {
    metacg::MCGLogger::instance().getConsole()->warn("Node does not have NumConditionalBranchMetaData");
    return 0;
  }
}

ConditionalBranchesEstimatorPhase::ConditionalBranchesEstimatorPhase(long int threshold, metacg::Callgraph *callgraph,
                                                                     StatisticsEstimatorPhase *prevStatEP)
    : SummingCountPhaseBase(threshold, "ConditionalBranches", callgraph, prevStatEP) {}

ConditionalBranchesReverseEstimatorPhase::ConditionalBranchesReverseEstimatorPhase(long int threshold,
                                                                                   metacg::Callgraph *callgraph,
                                                                                   StatisticsEstimatorPhase *prevStatEP)
    : SummingCountPhaseBase(threshold, "ConditionalBranchesReverse", callgraph, prevStatEP) {}
long int ConditionalBranchesReverseEstimatorPhase::getPreviousThreshold() const {
  return pSEP->getCuttoffReversesConditionalBranches();
}
long int ConditionalBranchesReverseEstimatorPhase::getTargetCount(const CgNode *node) const {
  if (node->has<NumConditionalBranchMetaData>()) {
    const auto md = node->get<NumConditionalBranchMetaData>();
    return maxBranches - md->numConditionalBranches;
  } else {
    metacg::MCGLogger::instance().getConsole()->warn("Node does not have NumConditionalBranchMetaData");
    return maxBranches;
  }
}
void ConditionalBranchesReverseEstimatorPhase::runInitialization() {
  maxBranches = 0;
  for (const auto &elem : graph->getNodes()) {
    const auto &node = elem.second.get();
    if (node->has<NumConditionalBranchMetaData>()) {
      const auto md = node->get<NumConditionalBranchMetaData>();
      maxBranches = std::max(maxBranches, static_cast<long int>(md->numConditionalBranches));
    } else {
      metacg::MCGLogger::instance().getConsole()->warn("Node does not have NumConditionalBranchMetaData");
    }
  }
}

FPAndMemOpsEstimatorPhase::FPAndMemOpsEstimatorPhase(long int threshold, metacg::Callgraph *callgraph,
                                                     StatisticsEstimatorPhase *prevStatEP)
    : SummingCountPhaseBase(threshold, "IntMemoryAccesses", callgraph, prevStatEP) {}
long int FPAndMemOpsEstimatorPhase::getPreviousThreshold() const { return pSEP->getCuttoffRoofline(); }
long int FPAndMemOpsEstimatorPhase::getTargetCount(const CgNode *node) const {
  if (node->has<NumOperationsMetaData>()) {
    const auto md = node->get<NumOperationsMetaData>();
    return md->numberOfFloatOps + md->numberOfMemoryAccesses;
  } else {
    metacg::MCGLogger::instance().getConsole()->warn("Node does not have NumOperationsMetaData");
    return 0;
  }
}

LoopDepthEstimatorPhase::LoopDepthEstimatorPhase(long int threshold, metacg::Callgraph *callgraph,
                                                 StatisticsEstimatorPhase *prevStatEP)
    : SummingCountPhaseBase(threshold, "LoopDepth", callgraph, prevStatEP, false) {}
long int LoopDepthEstimatorPhase::getPreviousThreshold() const { return pSEP->getCuttoffLoopDepth(); }
long int LoopDepthEstimatorPhase::getTargetCount(const CgNode *node) const {
  if (node->has<LoopDepthMetaData>()) {
    const auto md = node->get<LoopDepthMetaData>();
    return md->loopDepth;
  } else {
    metacg::MCGLogger::instance().getConsole()->warn("Node does not have LoopDepthMetaData");
    return 0;
  }
}

GlobalLoopDepthEstimatorPhase::GlobalLoopDepthEstimatorPhase(long int threshold, metacg::Callgraph *callgraph,
                                                             StatisticsEstimatorPhase *prevStatEP)
    : SummingCountPhaseBase(threshold, "GlobalLoopDepth", callgraph, prevStatEP, false) {}
long int GlobalLoopDepthEstimatorPhase::getPreviousThreshold() const { return pSEP->getCuttoffGlobalLoopDepth(); }
long int GlobalLoopDepthEstimatorPhase::getTargetCount(const CgNode *node) const {
  if (node->has<GlobalLoopDepthMetaData>()) {
    const auto md = node->get<GlobalLoopDepthMetaData>();
    return md->globalLoopDepth;
  } else {
    metacg::MCGLogger::instance().getConsole()->warn("Node does not have GlobalLoopDepthMetaData");
    return 0;
  }
}
void AttachInstrumentationResultsEstimatorPhase::modifyGraph(CgNode *mainMethod) {
  for (const auto &elem : graph->getNodes()) {
    const auto &node = elem.second.get();
    // We have pretty exact information about a function if we instrument it and all of its childs
    // This information should not change between iterations, so we do not need to overwrite/recalculate it
    const auto instResult = node->checkAndGet<InstrumentationResultMetaData>();
    if (instResult.first && (instResult.second->shouldBeInstrumented && !node->getOrCreateMD<PiraOneData>()->comesFromCube())) {
      // A node should have been instrumented, but was not in the cube file. This means it has zero calls and zero
      // runtime
      instResult.second->isExclusiveRuntime = true;
      instResult.second->shouldBeInstrumented = false;
      instResult.second->callCount = 0;
      instResult.second->callsFromParents = {};
      instResult.second->runtime = 0;
      instResult.second->timePerCall = 0;
      instResult.second->inclusiveRunTimeCube = 0;
      instResult.second->inclusiveRunTimeSum = 0;
      instResult.second->inclusiveTimePerCallCube = 0;
      instResult.second->inclusiveTimePerCallSum = 0;
      continue;
    }

    if (node->getOrCreateMD<PiraOneData>()->comesFromCube()) {
      // We already calculated an exclusive result before. Do not change it, as there could be small measurement
      // differences that could cause flickering between different instrumentation states
      bool hasPrevExclusive = instResult.first && instResult.second->isExclusiveRuntime;

      const auto runtime = node->get<BaseProfileData>()->getRuntimeInSeconds();
      const auto inclusiveRuntime = node->get<BaseProfileData>()->getInclusiveRuntimeInSeconds();
      const auto calls = node->get<BaseProfileData>()->getNumberOfCalls();
      const auto callsFromParents = node->get<BaseProfileData>()->getCallsFromParents();
      const auto &childs = graph->getCallees(node);
      const bool isExclusive = std::none_of(childs.begin(), childs.end(), [](const auto &child) {
        return !child->template getOrCreateMD<PiraOneData>()->comesFromCube();
      });
      // Calculate the summing inclusive runtime
      std::queue<CgNode *> workQueue;
      workQueue.push(node);
      std::set<CgNode *> visitedNodes;
      double inclusiveRuntimeSum = 0;
      while (!workQueue.empty()) {
        auto workNode = workQueue.front();
        workQueue.pop();
        if (visitedNodes.find(workNode) == visitedNodes.end()) {
          visitedNodes.insert(workNode);
          inclusiveRuntimeSum += workNode->get<BaseProfileData>()->getRuntimeInSeconds();
          for (const auto childNode : graph->getCallees(workNode)) {
            workQueue.push(childNode);
          }
        }
      }

      const auto md = node->getOrCreateMD<InstrumentationResultMetaData>();
      if (!hasPrevExclusive) {
        md->runtime = runtime;
        md->callCount = calls;
        md->isExclusiveRuntime = isExclusive;
        md->timePerCall = runtime / calls;
      }

      // This should not change, but better be safe
      md->callsFromParents = callsFromParents;
      // These can change if some calls called by a children get newly instrumented
      md->inclusiveRunTimeCube = inclusiveRuntime;
      md->inclusiveTimePerCallCube = inclusiveRuntime / calls;
      md->inclusiveRunTimeSum = inclusiveRuntimeSum;
      md->inclusiveTimePerCallSum = inclusiveRuntimeSum / calls;
    }
  }
}
AttachInstrumentationResultsEstimatorPhase::AttachInstrumentationResultsEstimatorPhase(metacg::Callgraph *callgraph)
    : EstimatorPhase("AttachInstrumentationResultsEstimatorPhase", callgraph) {}
void AttachInstrumentationResultsEstimatorPhase::printReport() {
  const auto comperator = [](const CgNode *lhs, const CgNode *rhs) {
    return lhs->get<InstrumentationResultMetaData>()->inclusiveRunTimeSum <
           rhs->get<InstrumentationResultMetaData>()->inclusiveRunTimeSum;
  };
  std::set<CgNode *, decltype(comperator)> snodes(comperator);
  for (const auto &elem : graph->getNodes()) {
    const auto &node = elem.second.get();
    if (node->has<InstrumentationResultMetaData>()) {
      snodes.insert(node);
    }
  }
  auto console = metacg::MCGLogger::instance().getConsole();
  console->debug("Begin report for {}", getName());
  for (const auto &node : snodes) {
    const auto irmd = node->get<InstrumentationResultMetaData>();
    if (irmd->isExclusiveRuntime) {
      console->debug(
          "{}: Calls: {} Time: {:.10f} Time per call: {:.10f} Inclusive runtime: {:.10f} (Exclusive) Inclusive runtime "
          "sum: {:.10f}",
          node->getFunctionName(), irmd->callCount, irmd->runtime, irmd->timePerCall, irmd->inclusiveRunTimeCube,
          irmd->inclusiveRunTimeSum);
    } else {
      console->debug(
          "{}: Calls: {} Time: {:.10f} Time per call: {:.10f} Inclusive runtime: {:.10f} (Not-Exclusive) Inclusive "
          "runtime "
          "sum: {:.10f}",
          node->getFunctionName(), irmd->callCount, irmd->runtime, irmd->timePerCall, irmd->inclusiveRunTimeCube,
          irmd->inclusiveRunTimeSum);
    }
  }
  // Reset precision
  console->debug("End report for {}", getName());
}
void FillInstrumentationGapsPhase::modifyGraph(CgNode *mainMethod) {
  metacg::analysis::ReachabilityAnalysis ra(graph);
  std::unordered_map<CgNode *, CgNodeRawPtrUSet> pathsToMain;

  for (const auto &elem : graph->getNodes()) {
    const auto &node = elem.second.get();
    const auto &parents = graph->getCallers(node);
    if (pgis::isAnyInstrumented(node) &&
        std::any_of(parents.begin(), parents.end(), [](const auto &p) { return !pgis::isAnyInstrumented(p); })) {
      auto nodesToMain = CgHelper::allNodesToMain(node, mainMethod, graph, pathsToMain, ra);
      pathsToMain[node] = nodesToMain;
      for (auto &ntm : nodesToMain) {
        if (!pgis::isAnyInstrumented(ntm)) {
          nodesToFill.insert(ntm);
        }
      }
    }
  }
  for (const auto &ntf : nodesToFill) {
    const auto mtd = ntf->checkAndGet<InstrumentationResultMetaData>();
    if (mtd.first && mtd.second->callCount == 0 && mtd.second->isExclusiveRuntime) {
      // This node will never be called
      continue;
    }

    if (ntf->getHasBody() || isMPIFunction(ntf) || (!onlyEligibleNodes && !useCSInstrumentation)) {
      metacg::MCGLogger::instance().getConsole()->trace("Instrumenting {} as node to main.", ntf->getFunctionName());
      pgis::instrumentNode(ntf);
    } else if (useCSInstrumentation) {
      if (onlyEligibleNodes) {
        if (isEligibleForPathInstrumentation(ntf, graph)) {
          metacg::MCGLogger::instance().getConsole()->trace("Instrumenting (cs) {} as node to main.",
                                                            ntf->getFunctionName());
          pgis::instrumentPathNode(ntf);
        }
      } else {
        metacg::MCGLogger::instance().getConsole()->trace("Instrumenting (cs) {} as node to main.",
                                                          ntf->getFunctionName());
        pgis::instrumentPathNode(ntf);
      }
    }
  }
}

FillInstrumentationGapsPhase::FillInstrumentationGapsPhase(metacg::Callgraph *callgraph)
    : EstimatorPhase("FillInstrumentationGapsPhase", callgraph) {
  useCSInstrumentation =
      pgis::config::GlobalConfig::get().getAs<bool>(pgis::options::useCallSiteInstrumentation.cliName);
  onlyEligibleNodes = pgis::config::GlobalConfig::get().getAs<bool>(pgis::options::onlyInstrumentEligibleNodes.cliName);
}

void FillInstrumentationGapsPhase::printReport() {
  auto console = metacg::MCGLogger::instance().getConsole();
  console->debug("Begin report for {}", getName());
  console->debug("Instrumented:");
  for (const auto &node : nodesToFill) {
    console->debug(" {}", node->getFunctionName());
  }
  console->debug("End report for {}", getName());
}

void StoreInstrumentationDecisionsPhase::modifyGraph(metacg::CgNode * /*mainMethod*/) {
  for (const auto &elem : graph->getNodes()) {
    const auto &node = elem.second.get();
    if (pgis::isAnyInstrumented(node)) {
      node->getOrCreateMD<InstrumentationResultMetaData>()->shouldBeInstrumented = true;
    } else {
      const auto md = node->checkAndGet<InstrumentationResultMetaData>();
      if (md.first) {
        md.second->shouldBeInstrumented = false;
      }
    }
  }
}

StoreInstrumentationDecisionsPhase::StoreInstrumentationDecisionsPhase(metacg::Callgraph *callgraph)
    : EstimatorPhase("StoreInstrumentationDecisionsPhase", callgraph) {}

/**
 * File: IPCGEstimatorPhase.cpp
 * License: Part of the MetaCG project. Licensed under BSD 3 clause license. See LICENSE.txt file at
 * https://github.com/tudasc/metacg/LICENSE.txt
 */

#include "IPCGEstimatorPhase.h"
#include "CgHelper.h"
#include "config/GlobalConfig.h"

#include "spdlog/sinks/stdout_color_sinks.h"
#include "spdlog/spdlog.h"

#include <unordered_map>

using namespace metacg;
using namespace pira;

FirstNLevelsEstimatorPhase::FirstNLevelsEstimatorPhase(int levels)
    : EstimatorPhase(std::string("FirstNLevels") + std::to_string(levels)), levels(levels) {}

FirstNLevelsEstimatorPhase::~FirstNLevelsEstimatorPhase() {}

void FirstNLevelsEstimatorPhase::modifyGraph(CgNodePtr mainMethod) { instrumentLevel(mainMethod, levels); }

void FirstNLevelsEstimatorPhase::instrumentLevel(CgNodePtr parentNode, int levelsLeft) {
  if (levelsLeft == 0) {
    return;
  }

  parentNode->setState(CgNodeState::INSTRUMENT_WITNESS);

  for (auto childNode : parentNode->getChildNodes()) {
    instrumentLevel(childNode, levelsLeft - 1);
  }
}

//// STATEMENT COUNT ESTIMATOR PHASE

StatementCountEstimatorPhase::StatementCountEstimatorPhase(int numberOfStatementsThreshold, bool inclusiveMetric,
                                                           StatisticsEstimatorPhase *prevStatEP)
    : EstimatorPhase((inclusiveMetric ? std::string("Incl-") : std::string("Excl-")) + std::string("StatementCount-") +
                     std::to_string(numberOfStatementsThreshold)),
      numberOfStatementsThreshold(numberOfStatementsThreshold),
      inclusiveMetric(inclusiveMetric),
      pSEP(prevStatEP) {}

StatementCountEstimatorPhase::~StatementCountEstimatorPhase() {}

void StatementCountEstimatorPhase::modifyGraph(CgNodePtr mainMethod) {
  if (pSEP) {
    numberOfStatementsThreshold = pSEP->getCuttoffNumInclStmts();
    // numberOfStatementsThreshold = pSEP->getMaxNumInclStmts() * .5;
    // numberOfStatementsThreshold = pSEP->getMedianNumInclStmts();
    spdlog::get("console")->debug("Changed count: now using {} as threshold", numberOfStatementsThreshold);
  }

  for (auto node : *graph) {
    spdlog::get("console")->trace("Processing node: {}", node->getFunctionName());
    if (!node->isReachable()) {
      spdlog::get("console")->trace("\tskipping.");
      continue;
    }
    spdlog::get("console")->trace("\testimating.");
    estimateStatementCount(node);
  }
}

void StatementCountEstimatorPhase::estimateStatementCount(CgNodePtr startNode) {
  int inclStmtCount = 0;
  if (inclusiveMetric) {
    // INCLUSIVE
    std::queue<CgNodePtr> workQueue;
    workQueue.push(startNode);
    std::set<CgNodePtr> visitedNodes;

    while (!workQueue.empty()) {
      auto node = workQueue.front();
      const auto nodePOD = node->get<PiraOneData>();
      workQueue.pop();

      visitedNodes.insert(node);

      inclStmtCount += nodePOD->getNumberOfStatements();

      for (auto childNode : node->getChildNodes()) {
        if (visitedNodes.find(childNode) == visitedNodes.end()) {
          if (childNode->isReachable()) {
            workQueue.push(childNode);
          }
        }
      }
    }
    inclStmtCounts[startNode] = inclStmtCount;
  } else {
    // EXCLUSIVE
    const auto snPOD = startNode->get<PiraOneData>();
    inclStmtCount = snPOD->getNumberOfStatements();
  }

  auto console = spdlog::get("console");
  console->trace("Function: {} >> InclStatementCount: {}", startNode->getFunctionName(), inclStmtCount);
  if (inclStmtCount >= numberOfStatementsThreshold) {
    console->trace("Function {} added to instrumentation list", startNode->getFunctionName());
    startNode->setState(CgNodeState::INSTRUMENT_WITNESS);
  }
  auto useCSInstr = pgis::config::GlobalConfig::get().getAs<bool>(pgis::options::useCallSiteInstrumentation.cliName);
  if (useCSInstr && /*!startNode->get<PiraOneData>()->getHasBody()*/ !startNode->getHasBody() &&
      startNode->get<BaseProfileData>()->getRuntimeInSeconds() == .0) {
    console->trace("Function {} added to instrumentation path", startNode->getFunctionName());
    startNode->setState(CgNodeState::INSTRUMENT_PATH);
  }
}

void MaxRuntimeSelectionStrategy::operator()(CgNodePtr node) {
  // If we have at least one child, initialize maxRtChild with the first child node
  CgNodePtr maxRtChild{(node->getChildNodes().empty() ? nullptr : *(node->getChildNodes().begin()))};

  for (auto childNode : node->getChildNodes()) {
    const auto childPOD = childNode->get<BaseProfileData>();
    const auto maxRtPOD = maxRtChild->get<BaseProfileData>();
    if (childPOD->getInclusiveRuntimeInSeconds() > maxRtPOD->getInclusiveRuntimeInSeconds()) {
      maxRtChild = childNode;
    }
  }
  // If we have a maxRtChild, this is not a leaf node.
  if (maxRtChild) {
    const auto maxRtPOD = maxRtChild->get<PiraOneData>();
    maxRtChild->setState(CgNodeState::INSTRUMENT_WITNESS);
    maxRtPOD->setDominantRuntime();
  } else {
    // This is a leaf node. Instrument the children
    if (node->get<PiraOneData>()->isDominantRuntime()) {
      for (auto child : node->getChildNodes()) {
        child->setState(CgNodeState::INSTRUMENT_WITNESS);
      }
    }
  }
}

void MaxStmtSelectionStrategy::operator()(CgNodePtr node) {
  std::map<CgNodePtr, size_t> childStmts;
  int maxStmts = 0;
  int totStmts = 0;
  CgNodePtr maxChild{nullptr};
  for (auto childNode : node->getChildNodes()) {
    StatementCountEstimatorPhase scep(999999999);  // prevent pass from instrumentation
    scep.estimateStatementCount(childNode);
    childStmts[childNode] = scep.getNumStatements(childNode);
    if (childStmts[childNode] > maxStmts) {
      maxStmts = childStmts[childNode];
      maxChild = childNode;
    }
    totStmts += childStmts[childNode];
  }
  int stmtThreshold = maxStmts * .3;
  float alpha = 1.0f;
  if (node->getChildNodes().size() > 0) {
    stmtThreshold = (totStmts / node->getChildNodes().size()) * alpha;
  } else {
    stmtThreshold = 0;
  }
  if (stmtThreshold < 1) {
    // This can happen, if all leaves are std lib functions.
    return;
  }
  for (auto childNode : node->getChildNodes()) {
    if (!childNode->isReachable()) {
      continue;
    }
    if (childNode->get<PiraOneData>()->getNumberOfStatements() >= stmtThreshold) {
      childNode->setState(CgNodeState::INSTRUMENT_WITNESS);
    }
  }
}

void RuntimeFilteredMixedStrategy::operator()(CgNodePtr node) {
  if (node->get<BaseProfileData>()->getInclusiveRuntimeInSeconds() > rtThresh) {
    for (auto child : node->getChildNodes()) {
      if (child->get<PiraOneData>()->comesFromCube()) {
        continue;
      }
      if (child->get<PiraOneData>()->getNumberOfStatements() > stmtThresh) {
        child->setState(CgNodeState::INSTRUMENT_WITNESS);
      }
    }
  }
}

void HybridEstimatorPhase::modifyGraph(CgNodePtr node) {
  for (auto node : *graph) {
    for (const auto s : strategies) {
      s->operator()(node);
    }
  }
}

// Runtime estimator phase

RuntimeEstimatorPhase::RuntimeEstimatorPhase(double runTimeThreshold, bool inclusiveMetric)
    : EstimatorPhase((inclusiveMetric ? std::string("Incl-") : std::string("Excl-")) + std::string("Runtime-") +
                     std::to_string(runTimeThreshold)),
      runTimeThreshold(runTimeThreshold),
      inclusiveMetric(inclusiveMetric) {}

RuntimeEstimatorPhase::~RuntimeEstimatorPhase() {}

void RuntimeEstimatorPhase::modifyGraph(CgNodePtr mainMethod) {
  for (auto node : *graph) {
    estimateRuntime(node);
  }

  runTimeThreshold = CgHelper::calcRuntimeThreshold(*graph, true);
  spdlog::get("console")->debug("The runtime is threshold is computed as: {}", runTimeThreshold);

  std::queue<CgNodePtr> workQueue;
  workQueue.push(mainMethod);
  CgNodePtrSet visitedNodes;

  while (!workQueue.empty()) {
    auto node = workQueue.front();
    workQueue.pop();

    visitedNodes.insert(node);
    doInstrumentation(node);

    for (auto childNode : node->getChildNodes()) {
      // Only visit unseen, profiled nodes. Only those have actual timing info!
      if (visitedNodes.find(childNode) == visitedNodes.end()) {
        workQueue.push(childNode);
      }
    }
  }
}

void RuntimeEstimatorPhase::estimateRuntime(CgNodePtr startNode) {
  double runTime = 0.0;

  if (inclusiveMetric) {
    // INCLUSIVE
    std::queue<CgNodePtr> workQueue;
    workQueue.push(startNode);
    CgNodePtrUnorderedSet visitedNodes;

    while (!workQueue.empty()) {
      auto node = workQueue.front();
      workQueue.pop();

      visitedNodes.insert(node);

      // Only count the runtime of nodes comming from the profile
      if (node->get<PiraOneData>()->comesFromCube()) {
        runTime += node->get<BaseProfileData>()->getRuntimeInSeconds();
      }

      for (auto childNode : node->getChildNodes()) {
        // Only visit unseen, profiled nodes. Only those have actual timing info!
        if (visitedNodes.find(childNode) == visitedNodes.end() && childNode->get<PiraOneData>()->comesFromCube()) {
          workQueue.push(childNode);
        }
      }
    }

    // inclRunTime[startNode] = runTime;
    inclRunTime[startNode] = startNode->get<BaseProfileData>()->getInclusiveRuntimeInSeconds();
  } else {
    // EXCLUSIVE
    runTime = startNode->get<BaseProfileData>()->getRuntimeInSeconds();
  }
}

void RuntimeEstimatorPhase::doInstrumentation(CgNodePtr startNode) {
  auto runTime = inclRunTime[startNode];
  spdlog::get("console")->debug(
      "Processing {}:\n\tNode RT:\t{}\n\tCalced RT:\t{}\n\tThreshold:\t{}", startNode->getFunctionName(),
      startNode->get<BaseProfileData>()->getInclusiveRuntimeInSeconds(), runTime, runTimeThreshold);
  if (runTime >= runTimeThreshold) {
    // keep the nodes on the paths in the profile, when they expose sufficient runtime.
    startNode->setState(CgNodeState::INSTRUMENT_WITNESS);
    spdlog::get("console")->info("Instrumenting {} because of its runtime", startNode->getFunctionName());
    int instrChildren = 0;

    std::map<CgNodePtr, long int> childStmts;
    long int maxStmts = 0;
    int totStmts = 0;
    // If we have at least one child, initialize maxRtChild with the first child node
    CgNodePtr maxRtChild{nullptr};

    for (auto childNode : startNode->getChildNodes()) {
      StatementCountEstimatorPhase scep(999999999);  // prevent pass from instrumentation
      scep.estimateStatementCount(childNode);
      childStmts[childNode] = scep.getNumStatements(childNode);

      if (childStmts[childNode] > maxStmts) {
        maxStmts = childStmts[childNode];
      }

      if (childNode->get<PiraOneData>()->comesFromCube()) {
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
      spdlog::get("console")->debug("The principal node set to {}", maxRtChild->getFunctionName());
    }

    int stmtThreshold = maxStmts;
    // XXX Original PIRA I value
    float alpha = .1f;

    /* PIRA I: simply uses the size of the children nodes container.
     * XXX: We should only consider children that have a body defines, i.e., that are eligible for instrumentation
     * with our method. This specifically excludes, for example, stdlib functions.
     */
    auto numChildren = startNode->getChildNodes().size();
#define NEW_PIRA_ONE 1
//#undef NEW_PIRA_ONE
#ifdef NEW_PIRA_ONE
    alpha = .3f;
    numChildren = std::count_if(std::begin(startNode->getChildNodes()), std::end(startNode->getChildNodes()),
                                [&](const auto &n) { return childStmts[n] > 0; });
#endif
    if (numChildren > 0) {
      stmtThreshold = (maxStmts / numChildren) * alpha;
      spdlog::get("console")->debug(
          "=== Children Info for {} ===\n\tMax Stmts:\t{}\n\tNum Children:\t{}\n\tStmt Threshold:\t{}",
          startNode->getFunctionName(), maxStmts, numChildren, stmtThreshold);
    } else {
      stmtThreshold = 1;
    }

    spdlog::get("console")->trace(
        ">>> Algorithm Infos <<<\n\tRuntime:\t{}\n\tMax Stmts:\t{}\n\tAlpha:\t\t{}\n\tTotal Stmts:\t{}\n\tStmt "
        "Threshold:\t{}\n\tInstr Children:\t{}\n",
        runTime, maxStmts, alpha, totStmts, stmtThreshold, instrChildren);

    if (stmtThreshold < 1) {
      // This can happen, if all leaves are std lib functions.
      spdlog::get("console")->debug("Statement Threshold < 1: Returning");
      return;
    }
    if (maxRtChild) {
      spdlog::get("console")->debug("This is the dominant runtime path");
      maxRtChild->setState(CgNodeState::INSTRUMENT_WITNESS);
      maxRtChild->get<PiraOneData>()->setDominantRuntime();
    } else {
      spdlog::get("console")->debug("This is the non-dominant runtime path");
      if (startNode->get<PiraOneData>()->isDominantRuntime()) {
        spdlog::get("console")->debug("\tPrincipal: {}", startNode->getFunctionName());
        for (auto child : startNode->getChildNodes()) {
          spdlog::get("console")->trace("\tEvaluating {} with {} [stmt threshold: {}]", child->getFunctionName(),
                                        childStmts[child], stmtThreshold);
          if (childStmts[child] > stmtThreshold) {
            child->setState(CgNodeState::INSTRUMENT_WITNESS);
            instrChildren++;
          }
        }
        std::cout << "\n\n";
      }
    }
    spdlog::get("console")->debug(
        "End of Processing {}:\n\tRuntime:\t{}\n\tMax Stmts:\t{}\n\tAlpha:\t\t{}\n\tTotal Stmt:\t{}\n\tStmt "
        "Threshold:\t{}\n\tInstr Children:\t{}",
        startNode->getFunctionName(), runTime, maxStmts, alpha, totStmts, stmtThreshold, instrChildren);
  }
}

void StatisticsEstimatorPhase::modifyGraph(CgNodePtr mainMethod) {
  numFunctions = graph->getGraph().size();
  // Threshold irrelevant, only building incl aggregation of interest
  StatementCountEstimatorPhase sce(999999999);
  sce.setGraph(graph);
  sce.modifyGraph(mainMethod);

  const auto heuristicMode = pgis::config::getSelectedHeuristic();
  switch (heuristicMode) {
    case pgis::options::HeuristicSelection::HeuristicSelectionEnum::STATEMENTS:
      // Statements get always collected
      break;
    case pgis::options::HeuristicSelection::HeuristicSelectionEnum::CONDITIONALBRANCHES: {
      ConditionalBranchesEstimatorPhase cbe(ConditionalBranchesEstimatorPhase::limitThreshold);
      cbe.setGraph(graph);
      cbe.modifyGraph(mainMethod);
    } break;
    case pgis::options::HeuristicSelection::HeuristicSelectionEnum::CONDITIONALBRANCHES_REVERSE: {
      ConditionalBranchesReverseEstimatorPhase cbre(ConditionalBranchesReverseEstimatorPhase::limitThreshold);
      cbre.setGraph(graph);
      cbre.modifyGraph(mainMethod);
    } break;
    case pgis::options::HeuristicSelection::HeuristicSelectionEnum::FP_MEM_OPS: {
      FPAndMemOpsEstimatorPhase re(FPAndMemOpsEstimatorPhase::limitThreshold);
      re.setGraph(graph);
      re.modifyGraph(mainMethod);
    } break;
    case pgis::options::HeuristicSelection::HeuristicSelectionEnum::LOOPDEPTH: {
      LoopDepthEstimatorPhase lde(LoopDepthEstimatorPhase::limitThreshold);
      lde.setGraph(graph);
      lde.modifyGraph(mainMethod);
    } break;
    case pgis::options::HeuristicSelection::HeuristicSelectionEnum::GlOBAL_LOOPDEPTH: {
      GlobalLoopDepthEstimatorPhase glde(GlobalLoopDepthEstimatorPhase::limitThreshold);
      glde.setGraph(graph);
      glde.modifyGraph(mainMethod);
    } break;
  }
  spdlog::get("console")->info("Running StatisticsEstimatorPhase::modifyGraph");

  for (const auto &node : *graph) {
    if (!node->isReachable()) {
      spdlog::get("console")->trace("Running on non-reachable function {}", node->getFunctionName());
      continue;
    }

    numReachableFunctions++;
    auto numStmts = node->get<PiraOneData>()->getNumberOfStatements();
    if (node->isInstrumentedWitness()) {
      stmtsCoveredWithInstr += numStmts;
    }
    if (node->get<PiraOneData>()->comesFromCube()) {
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
      spdlog::get("console")->warn("Node does not have CodeStatisticsMetaData");
    }
  }
}

void StatisticsEstimatorPhase::printReport() {
  if (!shouldPrintReport) {
    spdlog::get("console")->trace("Should not print report");
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

  spdlog::get("console")->info(
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
  spdlog::get("console")->info(printHist(stmtInclHist, "statements"));
  spdlog::get("console")->info(printHist(conditionalBranchesInclHist, "conditionalBranches"));
  spdlog::get("console")->info(printHist(reverseConditionalBranchesInclHist, "reverseConditionalBranches"));
  spdlog::get("console")->info(printHist(rooflineInclHist, "roofline"));
  spdlog::get("console")->info(printHist(loopDepthInclHist, "loopDepth"));
  spdlog::get("console")->info(printHist(globalLoopDepthInclHist, "globalLoopDepth"));
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
    : EstimatorPhase("WLCallpathDifferentiation"), whitelistName(whiteListName) {}

WLCallpathDifferentiationEstimatorPhase::~WLCallpathDifferentiationEstimatorPhase() {}

void WLCallpathDifferentiationEstimatorPhase::modifyGraph(CgNodePtr mainMethod) {
  // TODO: move this parsing somewhere else
  std::ifstream file(whitelistName);
  if (!file) {
    spdlog::get("errconsole")->error("Error in WLCallpathDifferentitation: Could not open {}", whitelistName);
    exit(1);
  }
  std::string line;
  while (std::getline(file, line)) {
    if (line.empty()) {
      continue;
    }
    CgNodePtr node = graph->getNode(line);
    if (node == nullptr) {
      continue;
    }
    addNodeAndParentsToWhitelist(node);
  }
  file.close();

  for (const auto &node : *graph) {
    if (CgHelper::isConjunction(node) && (whitelist.find(node) != whitelist.end())) {
      for (const auto &parentNode : node->getParentNodes()) {
        parentNode->setState(CgNodeState::INSTRUMENT_WITNESS);
      }
    }
  }
}

void WLCallpathDifferentiationEstimatorPhase::addNodeAndParentsToWhitelist(CgNodePtr node) {
  if (whitelist.find(node) == whitelist.end()) {
    whitelist.insert(node);

    for (const auto &parentNode : node->getParentNodes()) {
      addNodeAndParentsToWhitelist(parentNode);
    }
  }
}

SummingCountPhaseBase::~SummingCountPhaseBase() = default;

void SummingCountPhaseBase::modifyGraph(CgNodePtr mainMethod) {
  if (pSEP) {
    threshold = getPreviousThreshold();
    spdlog::get("console")->debug("Changed count: now using {} as threshold", threshold);
  }
  runInitialization();
  for (const auto &node : *graph) {
    spdlog::get("console")->trace("Processing node: {}", node->getFunctionName());
    if (!node->isReachable()) {
      spdlog::get("console")->trace("\tskipping.");
      continue;
    }
    spdlog::get("console")->trace("\testimating.");
    estimateCount(node);
  }
}
void SummingCountPhaseBase::estimateCount(const std::shared_ptr<CgNode> &startNode) {
  long int count = 0;
  // INCLUSIVE
  if (inclusive) {
    std::queue<CgNodePtr> workQueue;
    workQueue.push(startNode);
    std::set<CgNodePtr> visitedNodes;

    while (!workQueue.empty()) {
      auto node = workQueue.front();
      workQueue.pop();

      visitedNodes.insert(node);

      count += getTargetCount(node.get());

      for (const auto &childNode : node->getChildNodes()) {
        if (visitedNodes.find(childNode) == visitedNodes.end()) {
          if (childNode->isReachable()) {
            workQueue.push(childNode);
          }
        }
      }
    }
  } else {
    count += getTargetCount(startNode.get());
  }
  counts[startNode] = count;

  spdlog::get("console")->trace("Function: {} >> InclStatementCount: {}", startNode->getFunctionName(), count);
  if (count >= threshold) {
    startNode->setState(CgNodeState::INSTRUMENT_WITNESS);
  }
  if (/*!startNode->get<PiraOneData>()->getHasBody()*/ !startNode->getHasBody() &&
      startNode->get<BaseProfileData>()->getRuntimeInSeconds() == .0) {
    startNode->setState(CgNodeState::INSTRUMENT_PATH);
  }
}
SummingCountPhaseBase::SummingCountPhaseBase(long int threshold, const std::string &name,
                                             StatisticsEstimatorPhase *prevStatEP, bool inclusive)
    : EstimatorPhase((inclusive ? "Incl-" : "Excl-") + name + "-" + std::to_string(threshold)),
      threshold(threshold),
      pSEP(prevStatEP),
      inclusive(inclusive) {}
long int SummingCountPhaseBase::getCounted(const CgNodePtr &node) { return counts[node]; }
void SummingCountPhaseBase::runInitialization() {}

long int ConditionalBranchesEstimatorPhase::getPreviousThreshold() const {
  return pSEP->getCuttoffConditionalBranches();
}
long int ConditionalBranchesEstimatorPhase::getTargetCount(const CgNode *node) const {
  if (node->has<NumConditionalBranchMetaData>()) {
    const auto md = node->get<NumConditionalBranchMetaData>();
    return md->numConditionalBranches;
  } else {
    spdlog::get("console")->warn("Node does not have NumConditionalBranchMetaData");
    return 0;
  }
}
ConditionalBranchesEstimatorPhase::ConditionalBranchesEstimatorPhase(long int threshold,
                                                                     StatisticsEstimatorPhase *prevStatEP)
    : SummingCountPhaseBase(threshold, "ConditionalBranches", prevStatEP) {}

ConditionalBranchesReverseEstimatorPhase::ConditionalBranchesReverseEstimatorPhase(long int threshold,
                                                                                   StatisticsEstimatorPhase *prevStatEP)
    : SummingCountPhaseBase(threshold, "ConditionalBranchesReverse", prevStatEP) {}
long int ConditionalBranchesReverseEstimatorPhase::getPreviousThreshold() const {
  return pSEP->getCuttoffReversesConditionalBranches();
}
long int ConditionalBranchesReverseEstimatorPhase::getTargetCount(const CgNode *node) const {
  if (node->has<NumConditionalBranchMetaData>()) {
    const auto md = node->get<NumConditionalBranchMetaData>();
    return maxBranches - md->numConditionalBranches;
  } else {
    spdlog::get("console")->warn("Node does not have NumConditionalBranchMetaData");
    return maxBranches;
  }
}
void ConditionalBranchesReverseEstimatorPhase::runInitialization() {
  maxBranches = 0;
  for (const auto &node : *graph) {
    if (node->has<NumConditionalBranchMetaData>()) {
      const auto md = node->get<NumConditionalBranchMetaData>();
      maxBranches = std::max(maxBranches, static_cast<long int>(md->numConditionalBranches));
    } else {
      spdlog::get("console")->warn("Node does not have NumConditionalBranchMetaData");
    }
  }
}

FPAndMemOpsEstimatorPhase::FPAndMemOpsEstimatorPhase(long int threshold, StatisticsEstimatorPhase *prevStatEP)
    : SummingCountPhaseBase(threshold, "IntMemoryAccesses", prevStatEP) {}
long int FPAndMemOpsEstimatorPhase::getPreviousThreshold() const { return pSEP->getCuttoffRoofline(); }
long int FPAndMemOpsEstimatorPhase::getTargetCount(const CgNode *node) const {
  if (node->has<NumOperationsMetaData>()) {
    const auto md = node->get<NumOperationsMetaData>();
    return md->numberOfFloatOps + md->numberOfMemoryAccesses;
  } else {
    spdlog::get("console")->warn("Node does not have NumOperationsMetaData");
    return 0;
  }
}

LoopDepthEstimatorPhase::LoopDepthEstimatorPhase(long int threshold, StatisticsEstimatorPhase *prevStatEP)
    : SummingCountPhaseBase(threshold, "LoopDepth", prevStatEP, false) {}
long int LoopDepthEstimatorPhase::getPreviousThreshold() const { return pSEP->getCuttoffLoopDepth(); }
long int LoopDepthEstimatorPhase::getTargetCount(const CgNode *node) const {
  if (node->has<LoopDepthMetaData>()) {
    const auto md = node->get<LoopDepthMetaData>();
    return md->loopDepth;
  } else {
    spdlog::get("console")->warn("Node does not have LoopDepthMetaData");
    return 0;
  }
}

GlobalLoopDepthEstimatorPhase::GlobalLoopDepthEstimatorPhase(long int threshold, StatisticsEstimatorPhase *prevStatEP)
    : SummingCountPhaseBase(threshold, "GlobalLoopDepth", prevStatEP, false) {}
long int GlobalLoopDepthEstimatorPhase::getPreviousThreshold() const { return pSEP->getCuttoffGlobalLoopDepth(); }
long int GlobalLoopDepthEstimatorPhase::getTargetCount(const CgNode *node) const {
  if (node->has<GlobalLoopDepthMetaData>()) {
    const auto md = node->get<GlobalLoopDepthMetaData>();
    return md->globalLoopDepth;
  } else {
    spdlog::get("console")->warn("Node does not have GlobalLoopDepthMetaData");
    return 0;
  }
}


#include "IPCGEstimatorPhase.h"
#include "CgHelper.h"

#include "spdlog/sinks/stdout_color_sinks.h"
#include "spdlog/spdlog.h"

#include <unordered_map>

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
    numberOfStatementsThreshold = pSEP->getMaxNumInclStmts() * .5;
    numberOfStatementsThreshold = pSEP->getMedianNumInclStmts();
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

  spdlog::get("console")->trace("Function: {} >> InclStatementCount: {}", startNode->getFunctionName(), inclStmtCount);
  if (inclStmtCount >= numberOfStatementsThreshold) {
    startNode->setState(CgNodeState::INSTRUMENT_WITNESS);
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

    inclRunTime[startNode] = runTime;
    startNode->get<BaseProfileData>()->setInclusiveRuntimeInSeconds(runTime);
  } else {
    // EXCLUSIVE
    runTime = startNode->get<BaseProfileData>()->getRuntimeInSeconds();
  }
}

void RuntimeEstimatorPhase::doInstrumentation(CgNodePtr startNode) {
  auto runTime = inclRunTime[startNode];
  spdlog::get("console")->debug("Processing {}:\n\tNode RT:\t{}\n\tCalced RT:\t{}\n\tThreshold:\t{}",
                                startNode->getFunctionName(), startNode->get<BaseProfileData>()->getInclusiveRuntimeInSeconds(), runTime,
                                runTimeThreshold);
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
        } else if (childNode->get<BaseProfileData>()->getInclusiveRuntimeInSeconds() >= maxRtChild->get<BaseProfileData>()->getInclusiveRuntimeInSeconds()) {
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
  spdlog::get("console")->info("Running StatisticsEstimatorPhase::modifyGraph");

  for (auto node : *graph) {
    if (!node->isReachable()) {
      continue;
    }
    auto numStmts = node->get<PiraOneData>()->getNumberOfStatements();
    if (node->isInstrumentedWitness()) {
      // std::cout << "Processing reachable node " << node->getFunctionName() << " for statement count\n";
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
      " === Call graph statistics ===\nNo. of Functions:\t{}\nNo. of statements:\t{}\nMax No. Incl Stmt:\t{}\nMedian "
      "No. Incl Stmt:\t{}\nMin No. Incl Stmt:\t{}\nMax No. Stmt:\t\t{}\nMedian No. Stmt:\t{}\nMin No. "
      "Stmt:\t\t{}\nCovered w/ Instr:\t{}\nStmt not Covered:\t{}\n === === === === ===",
      numFunctions, totalStmts, maxNumStmts, medianNumStmts, minNumStmts, maxNumSingleStmts, medianNumSingleStmts,
      minNumSingleStmts, stmtsCoveredWithInstr, stmtsActuallyCovered, (totalStmts - stmtsCoveredWithInstr));
}

long int StatisticsEstimatorPhase::getMedianNumInclStmts() {
  const auto toRandAccContainer = [&](decltype(stmtHist) &inCont) {
    std::vector<std::remove_reference<decltype(inCont)>::type::key_type> keys;
    for (const auto &n : inCont) {
      keys.push_back(n.first);
    }
    return keys;
  };

  auto inclStmts = toRandAccContainer(stmtInclHist);
  std::nth_element(inclStmts.begin(), inclStmts.begin() + inclStmts.size() / 2, inclStmts.end());

  long int medianNumStmts = inclStmts[inclStmts.size() / 2];
  return medianNumStmts;
}

long int StatisticsEstimatorPhase::getMaxNumInclStmts() {
  const auto toRandAccContainer = [&](decltype(stmtHist) &inCont) {
    std::vector<std::remove_reference<decltype(inCont)>::type::key_type> keys;
    for (const auto &n : inCont) {
      keys.push_back(n.first);
    }
    return keys;
  };

  auto inclStmts = toRandAccContainer(stmtInclHist);
  return *std::max_element(inclStmts.begin(), inclStmts.end());
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
    CgNodePtr node = graph->findNode(line);
    if (node == nullptr) {
      continue;
    }
    addNodeAndParentsToWhitelist(node);
  }
  file.close();

  for (auto node : *graph) {
    if (CgHelper::isConjunction(node) && (whitelist.find(node) != whitelist.end())) {
      for (auto parentNode : node->getParentNodes()) {
        parentNode->setState(CgNodeState::INSTRUMENT_WITNESS);
      }
    }
  }
}

void WLCallpathDifferentiationEstimatorPhase::addNodeAndParentsToWhitelist(CgNodePtr node) {
  if (whitelist.find(node) == whitelist.end()) {
    whitelist.insert(node);

    for (auto parentNode : node->getParentNodes()) {
      addNodeAndParentsToWhitelist(parentNode);
    }
  }
}

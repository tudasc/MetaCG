
#include "IPCGEstimatorPhase.h"
#include "CgHelper.h"

#include <unordered_map>

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
    : EstimatorPhase((inclusiveMetric ? std::string("Incl") : std::string("Excl")) + std::string("StatementCount") +
                     std::to_string(numberOfStatementsThreshold)),
      numberOfStatementsThreshold(numberOfStatementsThreshold),
      inclusiveMetric(inclusiveMetric),
      pSEP(prevStatEP) {}

StatementCountEstimatorPhase::~StatementCountEstimatorPhase() {}

void StatementCountEstimatorPhase::modifyGraph(CgNodePtr mainMethod) {
  if (pSEP) {
    numberOfStatementsThreshold = pSEP->getMaxNumInclStmts() * .5;
    numberOfStatementsThreshold = pSEP->getMedianNumInclStmts();
    std::cout << "CHANGED COUNT: NOW USING " << numberOfStatementsThreshold << " AS THRESHOLD!" << std::endl;
  }

  for (auto node : *graph) {
    if (!node->isReachable()) {
      continue;
    }
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
      workQueue.pop();

      visitedNodes.insert(node);

      inclStmtCount += node->getNumberOfStatements();

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
    inclStmtCount = startNode->getNumberOfStatements();
  }

  // std::cout << startNode->getFunctionName() << ": " << inclStmtCount << "\n";
  if (inclStmtCount >= numberOfStatementsThreshold) {
    startNode->setState(CgNodeState::INSTRUMENT_WITNESS);
  }
}

void MaxRuntimeSelectionStrategy::operator()(CgNodePtr node) {
  // If we have at least one child, initialize maxRtChild with the first child node
  CgNodePtr maxRtChild{(node->getChildNodes().empty() ? nullptr : *(node->getChildNodes().begin()))};

  for (auto childNode : node->getChildNodes()) {
    if (childNode->getInclusiveRuntimeInSeconds() > maxRtChild->getInclusiveRuntimeInSeconds()) {
      maxRtChild = childNode;
    }
  }
  // If we have a maxRtChild, this is not a leaf node.
  if (maxRtChild) {
    maxRtChild->setState(CgNodeState::INSTRUMENT_WITNESS);
    maxRtChild->setDominantRuntime();
  } else {
    // This is a leaf node. Instrument the children
    if (node->isDominantRuntime()) {
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
    //std::cout << "Node " << node->getFunctionName() << " has Extra-P model: " << childNode->getEPMetric() << std::endl;
    if (!childNode->isReachable()) {
      continue;
    }
    if (childNode->getNumberOfStatements() >= stmtThreshold) {
      childNode->setState(CgNodeState::INSTRUMENT_WITNESS);
    }
  }
}

void RuntimeFilteredMixedStrategy::operator()(CgNodePtr node) {
  if (node->getInclusiveRuntimeInSeconds() > rtThresh) {
    for (auto child : node->getChildNodes()) {
      if (child->comesFromCube()) {
        continue;
      }
      if (child->getNumberOfStatements() > stmtThresh) {
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
    : EstimatorPhase((inclusiveMetric ? std::string("Incl") : std::string("Excl")) + std::string("Runtime") +
                     std::to_string(runTimeThreshold)),
      runTimeThreshold(runTimeThreshold),
      inclusiveMetric(inclusiveMetric) {}

RuntimeEstimatorPhase::~RuntimeEstimatorPhase() {}

void RuntimeEstimatorPhase::modifyGraph(CgNodePtr mainMethod) {
  for (auto node : *graph) {
    estimateRuntime(node);
  }

  runTimeThreshold = CgHelper::calcRuntimeThreshold(*graph, true);
  std::cout << "The runtime threshold is now: " << runTimeThreshold << "\n";

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
      if (node->comesFromCube()) {
        runTime += node->getRuntimeInSeconds();
      }

      for (auto childNode : node->getChildNodes()) {
        // Only visit unseen, profiled nodes. Only those have actual timing info!
        if (visitedNodes.find(childNode) == visitedNodes.end() && childNode->comesFromCube()) {
          workQueue.push(childNode);
        }
      }
    }

    inclRunTime[startNode] = runTime;
    startNode->setInclusiveRuntimeInSeconds(runTime);
  } else {
    // EXCLUSIVE
    runTime = startNode->getRuntimeInSeconds();
  }
}

void RuntimeEstimatorPhase::doInstrumentation(CgNodePtr startNode) {
  auto runTime = inclRunTime[startNode];
  if (runTime > runTimeThreshold) {
#ifdef NO_DEBUG
    std::cout << "Processing: " << startNode->getFunctionName() << ":\nNode runtime:\t\t"
              << startNode->getInclusiveRuntimeInSeconds() << "\nCalculated Runtime:\t" << runTime
              << " | threshold: " << runTimeThreshold << "\n";
#endif
    // keep the nodes on the paths in the profile, when they expose sufficient runtime.
    startNode->setState(CgNodeState::INSTRUMENT_WITNESS);
    std::cout << "[PGIS][Instrumenting] Node: " << startNode->getFunctionName();
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

      if (childNode->comesFromCube()) {
        if (childNode->getInclusiveRuntimeInSeconds() < runTimeThreshold) {
          continue;
        }
        if (maxRtChild == nullptr) {
          if (childNode == startNode) {
            continue;
          }
          maxRtChild = childNode;
        } else if (maxRtChild->getFunctionName() == childNode->getFunctionName() && maxRtChild != childNode) {
          // std::cerr << "\nWARNING NAMES EQUAL< POINTERS NOT\n";
        } else if (childNode->getInclusiveRuntimeInSeconds() >= maxRtChild->getInclusiveRuntimeInSeconds()) {
          if (childNode == startNode) {
            continue;
          }
          maxRtChild = childNode;
        }
      }
      totStmts += childStmts[childNode];
    }

    if (maxRtChild) {
      // std::cout << "\nDOMCHILD: " << maxRtChild->getFunctionName() << "\n";
    }

    int stmtThreshold = maxStmts * .3;
    float alpha = .1f;
    if (startNode->getChildNodes().size() > 0) {
      stmtThreshold = (maxStmts / startNode->getChildNodes().size()) * alpha;
    } else {
      stmtThreshold = 1;
    }
    if (stmtThreshold < 1) {
      // This can happen, if all leaves are std lib functions.
      std::cerr << "WARNING: RETURNING\n";
      return;
    }
    if (maxRtChild) {
      maxRtChild->setState(CgNodeState::INSTRUMENT_WITNESS);
      maxRtChild->setDominantRuntime();
    } else {
      if (startNode->isDominantRuntime()) {
        // std::cout << "\tDOM: " << startNode->getFunctionName();
        for (auto child : startNode->getChildNodes()) {
          // std::cout << "\n\t" << child->getFunctionName() << " | " << childStmts[child] << " | " << stmtThreshold;
          if (childStmts[child] > stmtThreshold) {
            child->setState(CgNodeState::INSTRUMENT_WITNESS);
            instrChildren++;
          }
        }
      }
    }
    std::cout << " | " << runTime << " | " << maxStmts << " | " << alpha << " | " << totStmts << " | " << stmtThreshold
              << " | " << instrChildren << "\n";
  }
}

void StatisticsEstimatorPhase::modifyGraph(CgNodePtr mainMethod) {
  numFunctions = graph->getGraph().size();
  // Threshold irrelevant, only building incl aggregation of interest
  StatementCountEstimatorPhase sce(999999999);
  sce.setGraph(graph);
  sce.modifyGraph(mainMethod);
  std::cout << "Running modify graph\n";

  for (auto node : *graph) {
    if (!node->isReachable()) {
      continue;
    }
    auto numStmts = node->getNumberOfStatements();
    if (node->isInstrumentedWitness()) {
      // std::cout << "Processing reachable node " << node->getFunctionName() << " for statement count\n";
      stmtsCoveredWithInstr += numStmts;
    }
    if (node->comesFromCube()) {
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
    std::cout << "Should not print report\n";
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

  std::cout << " === Call graph statistics ===\nNumber of Functions: " << numFunctions
            << "\nNumber of statements: " << totalStmts << "\n\nMax num incl statements: " << maxNumStmts
            << "\nMedian num incl statements: " << medianNumStmts << "\nMin num incl statements: " << minNumStmts
            << "\nMax num Stmts: " << maxNumSingleStmts << "\nMedian num statements: " << medianNumSingleStmts
            << "\nMin num statements: " << minNumSingleStmts
            << "\nCovered with Instrumentation: " << stmtsCoveredWithInstr
            << "\nCovered with Cube: " << stmtsActuallyCovered
            << "\nStmts not covered: " << (totalStmts - stmtsCoveredWithInstr) << "\n===============================\n";
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
    std::cerr << "Error in WLCallpathDifferentiation: Could not open " << whitelistName << std::endl;
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

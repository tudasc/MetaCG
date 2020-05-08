
#include "EstimatorPhase.h"

using namespace pira;

#define NO_DEBUG

EstimatorPhase::EstimatorPhase(std::string name, bool isMetaPhase)
    :

      graph(nullptr),  // just so eclipse does not nag
      report(),        // initializes all members of report
      name(name),
      config(nullptr),
      noReportRequired(isMetaPhase) {}

void EstimatorPhase::generateReport() {
  for (auto node : (*graph)) {
    const auto bpd = node->get<BaseProfileData>();
    if (node->isInstrumented()) {
      report.instrumentedMethods += 1;
      report.instrumentedCalls += bpd->getNumberOfCalls();

      report.instrumentedNames.insert(node->getFunctionName());
      report.instrumentedNodes.push(node);
    }
    if (node->isUnwound()) {
      unsigned long long unwindSamples = 0;
      if (node->isUnwoundInstr()) {
        unwindSamples = bpd->getNumberOfCalls();
      } else if (node->isUnwoundSample()) {
        unwindSamples = node->getExpectedNumberOfSamples();
      } else {
        std::cerr << "Error in generateRepor." << std::endl;
      }
      unsigned long long unwindSteps = node->getNumberOfUnwindSteps();

      unsigned long long unwindCostsNanos =
          unwindSamples * (CgConfig::nanosPerUnwindSample + unwindSteps * CgConfig::nanosPerUnwindStep);

      report.unwoundNames[node->getFunctionName()] = unwindSteps;

      report.unwindSamples += unwindSamples;
      report.unwindOvSeconds += (double)unwindCostsNanos / 1e9;

      report.unwConjunctions++;
    }
    if (CgHelper::isConjunction(node)) {
      report.overallConjunctions++;
    }
  }

  report.overallMethods = graph->size();

  report.instrOvSeconds = (double)report.instrumentedCalls * CgConfig::nanosPerInstrumentedCall / 1e9;

  if (config->referenceRuntime > .0) {
    report.instrOvPercent = report.instrOvSeconds / config->referenceRuntime * 100;
    report.unwindOvPercent = report.unwindOvSeconds / config->referenceRuntime * 100;

    report.samplesTaken = config->referenceRuntime * CgConfig::samplesPerSecond;
  } else {
    report.instrOvPercent = 0;
    report.samplesTaken = 0;
  }
  // sampling overhead
  if (!config->ignoreSamplingOv) {
    report.samplingOvSeconds = report.samplesTaken * CgConfig::nanosPerSample / 1e9;
    report.samplingOvPercent = (double)(CgConfig::nanosPerSample * CgConfig::samplesPerSecond) / 1e7;
  }

  report.overallSeconds = report.instrOvSeconds + report.unwindOvSeconds + report.samplingOvSeconds;
  report.overallPercent = report.instrOvPercent + report.unwindOvPercent + report.samplingOvPercent;

  report.metaPhase = noReportRequired;
  report.phaseName = name;

  if (!noReportRequired && report.overallPercent < config->fastestPhaseOvPercent) {
    config->fastestPhaseOvPercent = report.overallPercent;
    config->fastestPhaseOvSeconds = report.overallSeconds;
    config->fastestPhaseName = report.phaseName;
  }

  assert(report.instrumentedMethods == report.instrumentedNames.size());
  assert(report.instrumentedMethods == report.instrumentedNodes.size());
}

void EstimatorPhase::setGraph(Callgraph *graph) { this->graph = graph; }

CgReport EstimatorPhase::getReport() { return this->report; }

void EstimatorPhase::printReport() {
  if (noReportRequired) {
    return;
  }
  if (config->tinyReport) {
    if (!report.metaPhase) {
      double overallOvPercent = report.instrOvPercent + report.unwindOvPercent + report.samplingOvPercent;
      std::cout << "==" << report.phaseName << "==  " << overallOvPercent << " %" << std::endl;
    }
  } else {
    std::cout << "==" << report.phaseName << "==  " << std::endl;
    if (!report.metaPhase) {
      printAdditionalReport();
    }
  }
}

void EstimatorPhase::printAdditionalReport() {
  if (report.instrumentedCalls > 0) {
    std::cout << " INSTR \t" << std::setw(8) << std::left << report.instrOvPercent << " %"
              << " | instr. " << report.instrumentedMethods << " of " << report.overallMethods << " methods"
              << " | instrCalls: " << report.instrumentedCalls << " | instrOverhead: " << report.instrOvSeconds << " s"
              << std::endl;
  }
  if (report.unwindSamples > 0) {
    std::cout << "   UNW \t" << std::setw(8) << report.unwindOvPercent << " %"
              << " | unwound " << report.unwConjunctions << " of " << report.overallConjunctions << " conj."
              << " | unwindSamples: " << report.unwindSamples << " | undwindOverhead: " << report.unwindOvSeconds
              << " s" << std::endl;
  }
  if (!config->ignoreSamplingOv) {
    std::cout << " SAMPL \t" << std::setw(8) << report.samplingOvPercent << " %"
              << " | taken samples: " << report.samplesTaken << " | samplingOverhead: " << report.samplingOvSeconds
              << " s" << std::endl;
  }
  std::cout << " ---->\t" << std::setw(8) << report.overallPercent << " %"
            << " | overallOverhead: " << report.overallSeconds << " s" << std::endl;
}

//// REMOVE UNRELATED NODES ESTIMATOR PHASE

RemoveUnrelatedNodesEstimatorPhase::RemoveUnrelatedNodesEstimatorPhase(bool onlyRemoveUnrelatedNodes,
                                                                       bool aggressiveReduction)
    : EstimatorPhase("RemoveUnrelated", true),
      numUnconnectedRemoved(0),
      numLeafsRemoved(0),
      numChainsRemoved(0),
      numAdvancedOptimizations(0),
      aggressiveReduction(aggressiveReduction),
      onlyRemoveUnrelatedNodes(onlyRemoveUnrelatedNodes) {}

RemoveUnrelatedNodesEstimatorPhase::~RemoveUnrelatedNodesEstimatorPhase() { nodesToRemove.clear(); }

void RemoveUnrelatedNodesEstimatorPhase::modifyGraph(CgNodePtr mainMethod) {
  /* remove unrelated nodes (not reachable from main) */
  CgNodePtrSet nodesReachableFromMain = CgHelper::getDescendants(mainMethod);
  std::cout << "REACHABLE: " << nodesReachableFromMain.size() << std::endl;

  for (auto node : nodesReachableFromMain) {
    if (graph->getGraph().find(node) != graph->getGraph().end()) {
      node->setReachable();
    }
  }

  // FIXME: Is this actually problematic?
  return;

  for (auto node : (*graph)) {
    if (node == nullptr) {
      std::cout << "[DETECTED] Nullptr detected" << std::endl;
    }

    if (nodesReachableFromMain.find(node) == nodesReachableFromMain.end()) {
      graph->erase(node, false, true);

      numUnconnectedRemoved++;
    }
  }

  if (onlyRemoveUnrelatedNodes) {
    return;
  }

  /* remove leaf nodes */
  for (auto node : (*graph)) {
    if (node->isLeafNode()) {
      checkLeafNodeForRemoval(node);
    }
  }
  // actually remove those nodes
  for (auto node : nodesToRemove) {
    graph->erase(node);
  }

  /* remove linear chains */
  int oldNumChainsRemoved = 42;
  while (numChainsRemoved != oldNumChainsRemoved) {  // iterate until no change
    CgNodePtrQueueMostCalls pq(graph->begin(), graph->end());
    oldNumChainsRemoved = numChainsRemoved;

    for (auto node : Container(pq)) {
      if (node->hasUniqueChild() && node->hasUniqueParent()) {
        auto uniqueChild = node->getUniqueChild();
        auto nodeData = node->get<BaseProfileData>();
        auto childData = uniqueChild->get<BaseProfileData>();
        if (uniqueChild->hasUniqueParent() && (!CgHelper::isOnCycle(node) || node->hasUniqueChild())) {
          numChainsRemoved++;

          if (nodeData->getNumberOfCalls() >= childData->getNumberOfCalls()) {
            graph->erase(node, true);
          } else {
            graph->erase(uniqueChild, true);
          }
        }
      }
    }
  }

  if (!aggressiveReduction) {
    return;
  }

  CgNodePtrQueueMostCalls allNodes(graph->begin(), graph->end());
  for (auto node : Container(allNodes)) {
    // advanced optimization (remove node with subset of dependentConjunctions
    if (node->hasUniqueParent() && node->hasUniqueChild()) {
      auto uniqueParent = node->getUniqueParent();
      auto nodeData = node->get<BaseProfileData>();
      auto parentData = uniqueParent->get<BaseProfileData>();
      bool uniqueParentHasLessCalls = parentData->getNumberOfCalls() <= nodeData->getNumberOfCalls();
      bool uniqueParentServesMoreOrEqualsNodes =
          CgHelper::isSubsetOf(node->getDependentConjunctionsConst(), uniqueParent->getDependentConjunctionsConst());

      bool aggressiveReductionPossible = true;
      if (!uniqueParent->hasUniqueChild()) {
        for (auto childOfParent : uniqueParent->getChildNodes()) {
          if (childOfParent != node && CgHelper::intersects(node->getDependentConjunctionsConst(),
                                                            childOfParent->getDependentConjunctionsConst())) {
            aggressiveReductionPossible = false;
            break;
          }
        }
      }

      if (uniqueParentHasLessCalls && uniqueParentServesMoreOrEqualsNodes && aggressiveReductionPossible) {
        numAdvancedOptimizations++;
        graph->erase(node, true);
      }
    }
  }
}

void RemoveUnrelatedNodesEstimatorPhase::checkLeafNodeForRemoval(CgNodePtr potentialLeaf) {
  if (CgHelper::isConjunction(potentialLeaf)) {
    return;  // conjunctions are never removed
  }

  for (auto child : potentialLeaf->getChildNodes()) {
    if (nodesToRemove.find(child) == nodesToRemove.end()) {
      return;
    }
  }

  nodesToRemove.insert(potentialLeaf);
  numLeafsRemoved++;

  for (auto parentNode : potentialLeaf->getParentNodes()) {
    checkLeafNodeForRemoval(parentNode);
  }
}

void RemoveUnrelatedNodesEstimatorPhase::printAdditionalReport() {
  std::cout << "\t"
            << "Removed " << numUnconnectedRemoved << " unconnected node(s)." << std::endl;
  std::cout << "\t"
            << "Removed " << numLeafsRemoved << " leaf node(s)." << std::endl;
  std::cout << "\t"
            << "Removed " << numChainsRemoved << " node(s) in linear call chains." << std::endl;
  std::cout << "\t"
            << "Removed " << numAdvancedOptimizations << " node(s) in advanced optimization." << std::endl;
}

//// GRAPH STATS ESTIMATOR PHASE

GraphStatsEstimatorPhase::GraphStatsEstimatorPhase()
    : EstimatorPhase("GraphStats", true), numCyclesDetected(0), numberOfConjunctions(0) {}

GraphStatsEstimatorPhase::~GraphStatsEstimatorPhase() {}

void GraphStatsEstimatorPhase::modifyGraph(CgNodePtr mainMethod) {
  CgNodePtrSet nodesWithRemovedInstr;
  CgNodePtrSet unwoundNodes;

  for (auto node : (*graph)) {
    if (CgHelper::isConjunction(node)) {
      const auto nodeData = node->get<BaseProfileData>();
      unsigned long long unwindCostsNanos = nodeData->getNumberOfCalls() * CgConfig::nanosPerUnwindStep;
      unsigned long long instrCostsNanos = 0;
      for (auto parent : node->getParentNodes()) {
        const auto parentData = parent->get<BaseProfileData>();
        instrCostsNanos += parentData->getNumberOfCalls() * CgConfig::nanosPerInstrumentedCall;
      }

      if (unwindCostsNanos < instrCostsNanos) {
        unwoundNodes.insert(node);
        node->setState(CgNodeState::UNWIND_INSTR, 1);
        double secondsSaved = (double)(instrCostsNanos - unwindCostsNanos) / 1e9;
        for (auto parent : node->getParentNodes()) {
          nodesWithRemovedInstr.insert(parent);
        }
        if (secondsSaved > 1.) {
          std::cout << "max save: " << secondsSaved << " s in " << node->getFunctionName() << std::endl;
        }
      }
    }
  }

  unsigned long long nanosInstrCosts = 0;
  for (auto notInstrNode : nodesWithRemovedInstr) {
    const auto nodeData = notInstrNode->get<BaseProfileData>();
    nanosInstrCosts += nodeData->getNumberOfCalls() * CgConfig::nanosPerInstrumentedCall;
  }
  unsigned long long nanosUnwCosts = 0;
  for (auto unwNode : unwoundNodes) {
    const auto nodeData = unwNode->get<BaseProfileData>();
    nanosUnwCosts += nodeData->getNumberOfCalls() * CgConfig::nanosPerUnwindStep;
  }
  double secondsSavedOverall = (double)(nanosInstrCosts - nanosUnwCosts) / 1e9;
  std::cout << "overall save is: " << secondsSavedOverall << " s in " << unwoundNodes.size() << " functions "
            << std::endl;

}

void GraphStatsEstimatorPhase::printAdditionalReport() {
  std::cout << "\t"
            << "nodes in cycles: " << numCyclesDetected << std::endl;
  std::cout << "\t"
            << "numberOfConjunctions: " << numberOfConjunctions
            << " | allValidMarkerPositions: " << allValidMarkerPositions.size() << std::endl;
  for (auto dependency : dependencies) {
    std::cout << "\t- dependentConjunctions: " << std::setw(3) << dependency.dependentConjunctions.size()
              << " | validMarkerPositions: " << std::setw(3) << dependency.markerPositions.size() << std::endl;
  }
}

//// OVERHEAD COMPENSATION ESTIMATOR PHASE

OverheadCompensationEstimatorPhase::OverheadCompensationEstimatorPhase(int nanosPerHalpProbe)
    : EstimatorPhase("OvCompensation", false),
      nanosPerHalfProbe(nanosPerHalpProbe),
      overallRuntime(0),
      numOvercompensatedFunctions(0),
      numOvercompensatedCalls(0) {}

void OverheadCompensationEstimatorPhase::modifyGraph(CgNodePtr mainMethod) {
  for (auto node : *graph) {
    auto nodeData = node->get<BaseProfileData>();
    auto oldRuntime = nodeData->getRuntimeInSeconds();
    unsigned long long numberOfOwnOverheads = nodeData->getNumberOfCalls();
    unsigned long long numberOfChildOverheads = 0;
    for (auto child : node->getChildNodes()) {
      const auto childData = child->get<BaseProfileData>();
      numberOfChildOverheads += childData->getNumberOfCalls(node);
    }

    unsigned long long timestampOverheadNanos =
        numberOfOwnOverheads * nanosPerHalfProbe + numberOfChildOverheads * nanosPerHalfProbe;
    double timestampOverheadSeconds = (double)timestampOverheadNanos / 1e9;
    double newRuntime = oldRuntime - timestampOverheadSeconds;
    if (newRuntime < 0) {
      nodeData->setRuntimeInSeconds(0);
      numOvercompensatedFunctions++;
      numOvercompensatedCalls += nodeData->getNumberOfCalls();
    } else {
      nodeData->setRuntimeInSeconds(newRuntime);
    }

    if (config->samplesFile.empty()) {
      node->updateExpectedNumberOfSamples();
    }
    overallRuntime += newRuntime;
  }

  config->actualRuntime = overallRuntime;
}

void OverheadCompensationEstimatorPhase::printAdditionalReport() {
  std::cout << "\t"
            << "new runtime in seconds: " << overallRuntime << " | overcompensated: " << numOvercompensatedFunctions
            << " functions with " << numOvercompensatedCalls << " calls." << std::endl;
}

//// DIAMOND PATTERN SOLVER ESTIMATOR PHASE

DiamondPatternSolverEstimatorPhase::DiamondPatternSolverEstimatorPhase()
    : EstimatorPhase("DiamondPattern"), numDiamonds(0), numUniqueConjunction(0), numOperableConjunctions(0) {}

DiamondPatternSolverEstimatorPhase::~DiamondPatternSolverEstimatorPhase() {}

void DiamondPatternSolverEstimatorPhase::modifyGraph(CgNodePtr mainMethod) {
  for (auto node : (*graph)) {
    if (CgHelper::isConjunction(node)) {
      for (auto parent1 : node->getParentNodes()) {
        if (parent1->hasUniqueParent() && parent1->hasUniqueChild()) {
          auto grandParent = parent1->getUniqueParent();

          for (auto parent2 : node->getParentNodes()) {
            if (parent1 == parent2) {
              continue;
            }

            if (parent2->hasUniqueParent() && parent2->getUniqueParent() == grandParent) {
              numDiamonds++;
              break;
            }
          }
        }
      }
    }
  }

  // conjunctions with a unique solution
  for (auto &node : (*graph)) {
    if (CgHelper::isConjunction(node)) {
      int numParents = node->getParentNodes().size();
      int numPossibleMarkerPositions = node->getMarkerPositions().size();

      assert(numPossibleMarkerPositions >= (numParents - 1));

      if (numPossibleMarkerPositions == (numParents - 1)) {
        /// XXX instrument parents
        for (auto &marker : node->getMarkerPositions()) {
          marker->setState(CgNodeState::INSTRUMENT_WITNESS);
        }

        numUniqueConjunction++;
      }

      if (numPossibleMarkerPositions == numParents) {
        numOperableConjunctions++;
      }
    }
  }
}

void DiamondPatternSolverEstimatorPhase::printAdditionalReport() {
  std::cout << "\t"
            << "numberOfDiamonds: " << numDiamonds << std::endl;
  std::cout << "\t"
            << "numUniqueConjunction: " << numUniqueConjunction << std::endl;
  std::cout << "\t"
            << "numOperableConjunctions: " << numOperableConjunctions << std::endl;
}

//// INSTRUMENT ESTIMATOR PHASE

InstrumentEstimatorPhase::InstrumentEstimatorPhase(bool instrumentAll)
    : EstimatorPhase(instrumentAll ? "ss-all" : "ss-cpd"), instrumentAll(instrumentAll) {}

InstrumentEstimatorPhase::~InstrumentEstimatorPhase() {}

void InstrumentEstimatorPhase::modifyGraph(CgNodePtr mainMethod) {
  for (auto node : (*graph)) {
    if (instrumentAll) {
      node->setState(CgNodeState::INSTRUMENT_WITNESS);
    } else {
      if (CgHelper::isConjunction(node)) {
        for (auto nodeToInstrument : node->getParentNodes()) {
          nodeToInstrument->setState(CgNodeState::INSTRUMENT_WITNESS);
        }
      }
    }
  }
}

//// WL INSTR ESTIMATOR PHASE

WLInstrEstimatorPhase::WLInstrEstimatorPhase(std::string wlFilePath) : EstimatorPhase("WLInstr") {
  std::ifstream ifStream(wlFilePath);
  if (!ifStream.good()) {
    std::cerr << "Error: can not find whitelist at .. " << wlFilePath << std::endl;
  }

  std::string buff;
  while (getline(ifStream, buff)) {
    whiteList.insert(buff);
  }
}

void WLInstrEstimatorPhase::modifyGraph(CgNodePtr mainMethod) {
  for (auto node : (*graph)) {
    if (whiteList.find(node->getFunctionName()) != whiteList.end()) {
      node->setState(CgNodeState::INSTRUMENT_WITNESS);
    }
  }
}

//// LIBUNWIND ESTIMATOR PHASE

LibUnwindEstimatorPhase::LibUnwindEstimatorPhase(bool unwindUntilUniqueCallpath)
    : EstimatorPhase(unwindUntilUniqueCallpath ? "unw-min" : "unw-all"),
      currentDepth(0),
      unwindUntilUniqueCallpath(unwindUntilUniqueCallpath) {}

void LibUnwindEstimatorPhase::modifyGraph(CgNodePtr mainMethod) {
  // find max distance from main for every function
  for (auto child : mainMethod->getChildNodes()) {
    visit(mainMethod, child);
  }
}

void LibUnwindEstimatorPhase::visit(CgNodePtr from, CgNodePtr current) {
  if (!(unwindUntilUniqueCallpath && current->hasUniqueCallPath())) {
    currentDepth++;
  }

  visitedEdges.insert(CgEdge(from, current));

  /// XXX
  //	for (auto v : visitedEdges) {
  //		std::cout << v << std::endl;
  //	}
  //	std::cout << std::endl;

  auto unwindSteps = current->getNumberOfUnwindSteps();
  if (currentDepth > unwindSteps) {
    current->setState(CgNodeState::UNWIND_SAMPLE, currentDepth);
  }

  for (CgNodePtr child : current->getChildNodes()) {
    if (visitedEdges.find(CgEdge(current, child)) == visitedEdges.end()) {
      visit(current, child);
    }
  }

  if (config->greedyUnwind) {
    visitedEdges.erase(CgEdge(from, current));
  }
  if (!(unwindUntilUniqueCallpath && current->hasUniqueCallPath())) {
    currentDepth--;
  }
}

//// MOVE INSTRUMENTATION UPWARDS ESTIMATOR PHASE

MoveInstrumentationUpwardsEstimatorPhase::MoveInstrumentationUpwardsEstimatorPhase()
    : EstimatorPhase("MoveInstrumentationUpwards"), movedInstrumentations(0) {}

MoveInstrumentationUpwardsEstimatorPhase::~MoveInstrumentationUpwardsEstimatorPhase() {}

void MoveInstrumentationUpwardsEstimatorPhase::modifyGraph(CgNodePtr mainMethod) {
  for (auto node : (*graph)) {
    auto nextAncestor = node;

    // If the node was not selected previously, we continue
    if (!nextAncestor->isInstrumentedWitness()) {
      continue;
    }

    auto minimalCalls = nextAncestor;
    const auto minCallData = minimalCalls->get<BaseProfileData>();

    // If it was selected, look for a "cheaper" node upwards
    while (nextAncestor->hasUniqueParent()) {
      nextAncestor = nextAncestor->getUniqueParent();
      const auto naData = nextAncestor->get<BaseProfileData>();
      if (nextAncestor->getChildNodes().size() > 1) {
        break;  // don't move if parent has multiple children
      }

      if (naData->getNumberOfCalls() < minCallData->getNumberOfCalls()) {
        minimalCalls = nextAncestor;
      }
    }

    if (!minimalCalls->isSameFunction(nextAncestor)) {
      node->setState(CgNodeState::NONE);
      minimalCalls->setState(CgNodeState::INSTRUMENT_WITNESS);
      movedInstrumentations++;
    }
  }
}

//// DELETE ONE INSTRUMENTATION ESTIMATOR PHASE

MinInstrHeuristicEstimatorPhase::MinInstrHeuristicEstimatorPhase()
    : EstimatorPhase("ss-min"), deletedInstrumentationMarkers(0) {}

MinInstrHeuristicEstimatorPhase::~MinInstrHeuristicEstimatorPhase() {}

void MinInstrHeuristicEstimatorPhase::modifyGraph(CgNodePtr mainMethod) {
  CgNodePtrQueueMostCalls pq;
  for (auto node : (*graph)) {
    if (node->isInstrumented()) {
      pq.push(node);
    }
  }

  for (auto instrumentedNode : Container(pq)) {
    bool canDeleteInstruentation = true;
    for (auto node : (*graph)) {
      if (!CgHelper::isUniquelyInstrumented(node, instrumentedNode, false)) {
        canDeleteInstruentation = false;
        break;
      }
    }
    if (canDeleteInstruentation) {
      instrumentedNode->setState(CgNodeState::NONE);
      deletedInstrumentationMarkers++;
    }
  }
}

void MinInstrHeuristicEstimatorPhase::printAdditionalReport() {
  EstimatorPhase::printAdditionalReport();
  if (!config->tinyReport) {
    std::cout << "\t"
              << "deleted " << deletedInstrumentationMarkers << " instrumentation marker(s)" << std::endl;
  }
}

//// CONJUNCTION ESTIMATOR PHASE

ConjunctionInstrumentOnlyEstimatorPhase::ConjunctionInstrumentOnlyEstimatorPhase() : EstimatorPhase("ConjInstrOnly") {}

ConjunctionInstrumentOnlyEstimatorPhase::~ConjunctionInstrumentOnlyEstimatorPhase() {}

void ConjunctionInstrumentOnlyEstimatorPhase::modifyGraph(CgNodePtr mainMethod) {
  for (auto node : (*graph)) {
    if (CgHelper::isConjunction(node)) {
      node->setState(CgNodeState::INSTRUMENT_CONJUNCTION);
    }
  }
}

ConjunctionInstrumentHeuristicEstimatorPhase::ConjunctionInstrumentHeuristicEstimatorPhase()
    : EstimatorPhase("ss-conj") {}

ConjunctionInstrumentHeuristicEstimatorPhase::~ConjunctionInstrumentHeuristicEstimatorPhase() {}

void ConjunctionInstrumentHeuristicEstimatorPhase::modifyGraph(CgNodePtr mainMethod) {
  CgNodePtrQueueMostCalls pq(graph->begin(), graph->end());
  for (auto node : Container(pq)) {
    if (CgHelper::isConjunction(node)) {
      const auto nodeData = node->get<BaseProfileData>();
      unsigned long long conjInstrCosts = nodeData->getNumberOfCalls() * CgConfig::nanosPerInstrumentedCall;

      unsigned long long expectedInstrumentationOverheadNanos = CgHelper::getInstrumentationOverheadOfConjunction(node);
      unsigned long long expectedActualInstrumentationSavedNanos =
          CgHelper::getInstrumentationOverheadServingOnlyThisConjunction(node);
      unsigned long long witnessIntrsCostsSaved = expectedInstrumentationOverheadNanos;
      //						(expectedInstrumentationOverheadNanos
      //+ expectedActualInstrumentationSavedNanos) / 2;

      if (conjInstrCosts < witnessIntrsCostsSaved) {
        node->setState(CgNodeState::INSTRUMENT_CONJUNCTION);
      }

      // TODO remove substituted instr
      for (auto parentNode : node->getParentNodes()) {
        CgHelper::deleteInstrumentationIfRedundant(parentNode);
      }
    }
  }
}

//// UNWIND ESTIMATOR PHASE

UnwindEstimatorPhase::UnwindEstimatorPhase(bool unwindOnlyLeafNodes, bool unwindInInstr)
    : EstimatorPhase(unwindOnlyLeafNodes ? (unwindInInstr ? "UnwindInstrLeaf" : "UnwindSampleLeaf")
                                         : (unwindInInstr ? "UnwindInstr" : "hybrid-dyn")),
      numUnwoundNodes(0),
      unwindCandidates(0),
      unwindOnlyLeafNodes(unwindOnlyLeafNodes),
      unwindInInstr(unwindInInstr) {}

UnwindEstimatorPhase::~UnwindEstimatorPhase() {}

/** XXX never call this if there are cycles in the successors of the start node
 */
void UnwindEstimatorPhase::getNewlyUnwoundNodes(std::map<CgNodePtr, int> &unwoundNodes, CgNodePtr startNode,
                                                int unwindSteps) {
  if (unwoundNodes.find(startNode) == unwoundNodes.end() || unwoundNodes[startNode] < unwindSteps) {
    unwoundNodes[startNode] = unwindSteps;
  }
  for (auto child : startNode->getChildNodes()) {
    getNewlyUnwoundNodes(unwoundNodes, child, unwindSteps + 1);
  }
}

bool UnwindEstimatorPhase::canBeUnwound(CgNodePtr startNode) {
  if (unwindOnlyLeafNodes && !startNode->isLeafNode()) {
    return false;
  }

  for (auto node : CgHelper::getDescendants(startNode)) {
    if (CgHelper::isOnCycle(node)) {
      return false;
    }
  }
  return true;
}

unsigned long long UnwindEstimatorPhase::getUnwindOverheadNanos(std::map<CgNodePtr, int> &unwoundNodes) {
  unsigned long long unwindSampleOverheadNanos = 0;
  for (auto pair : unwoundNodes) {
    int numUnwindSteps = pair.second;
    int numExistingUnwindSteps = pair.first->getNumberOfUnwindSteps();

    int numNewUnwindSteps = 0;
    if (numUnwindSteps > numExistingUnwindSteps) {
      numNewUnwindSteps = numUnwindSteps - numExistingUnwindSteps;
    }

    unsigned long long numSamples = pair.first->getExpectedNumberOfSamples();
    unwindSampleOverheadNanos += numSamples * numNewUnwindSteps * CgConfig::nanosPerUnwindStep;
  }

  return unwindSampleOverheadNanos;
}

unsigned long long UnwindEstimatorPhase::getInstrOverheadNanos(std::map<CgNodePtr, int> &unwoundNodes) {
  std::set<CgNodePtr> keySet;
  for (auto pair : unwoundNodes) {
    keySet.insert(pair.first);
  }

  // optimistic
  unsigned long long expectedInstrumentationOverheadNanos = CgHelper::getInstrumentationOverheadOfConjunction(keySet);
  // pessimistic
  unsigned long long expectedActualInstrumentationSavedNanos =
      CgHelper::getInstrumentationOverheadServingOnlyThisConjunction(keySet);

  return (expectedInstrumentationOverheadNanos + expectedActualInstrumentationSavedNanos) / 2;
}

void UnwindEstimatorPhase::modifyGraph(CgNodePtr mainMethod) {
#ifndef NO_DEBUG
  double overallSavedSeconds = .0;
#endif

  CgNodePtrQueueUnwHeur pq;
  for (auto node : (*graph)) {
    if (CgHelper::isConjunction(node) && canBeUnwound(node)) {
      pq.push(node);
    }
  }

  while (!pq.empty()) {
    auto node = pq.top();
    pq.pop();

    unwindCandidates++;

    std::map<CgNodePtr, int> unwoundNodes;
    getNewlyUnwoundNodes(unwoundNodes, node);

    unsigned long long unwindOverhead = getUnwindOverheadNanos(unwoundNodes);
    if (unwindInInstr) {
      const auto nodeData = node->get<BaseProfileData>();
      unwindOverhead = nodeData->getNumberOfCalls() * CgConfig::nanosPerUnwindStep;
    }

    unsigned long long instrumentationOverhead = getInstrOverheadNanos(unwoundNodes);
    if (unwindInInstr) {
      std::map<CgNodePtr, int> unwoundNode = {{node, 1}};
      instrumentationOverhead = getInstrOverheadNanos(unwoundNode);
    }

    if (unwindOverhead < instrumentationOverhead) {
#ifndef NO_DEBUG
      double expectedOverheadSavedSeconds =
          ((long long)instrumentationOverhead - (long long)unwindOverhead) / 1000000000.0;
      if (expectedOverheadSavedSeconds > 0.1) {
        if (!node->isLeafNode()) {
          std::cout << "No Leaf - " << unwoundNodes.size() << " nodes unwound" << std::endl;
        }
        std::cout << std::setprecision(4) << expectedOverheadSavedSeconds
                  << "s\t save expected in: " << node->getFunctionName() << std::endl;
      }
      overallSavedSeconds += expectedOverheadSavedSeconds;
#endif

      if (unwindInInstr) {
        node->setState(CgNodeState::UNWIND_INSTR, 1);
        numUnwoundNodes++;
      } else {
        for (auto pair : unwoundNodes) {
          int numExistingUnwindSteps = pair.first->getNumberOfUnwindSteps();
          if (numExistingUnwindSteps == 0) {
            numUnwoundNodes++;
          }
          if (pair.second > numExistingUnwindSteps) {
            pair.first->setState(CgNodeState::UNWIND_SAMPLE, pair.second);
          }
        }
      }

      // remove redundant instrumentation in direct parents
      for (auto pair : unwoundNodes) {
        for (auto parentNode : pair.first->getParentNodes()) {
          CgHelper::deleteInstrumentationIfRedundant(parentNode);
        }
      }
    }
  }

#ifndef NO_DEBUG
  std::cout << overallSavedSeconds << " s maximum save through unwinding." << std::endl;
#endif
}

//// UNWIND SPECIAL ESTIMATOR PHASE

UnwStaticLeafEstimatorPhase::UnwStaticLeafEstimatorPhase() : EstimatorPhase("hybrid-st") {}

void UnwStaticLeafEstimatorPhase::modifyGraph(CgNodePtr mainMethod) {
  for (auto node : (*graph)) {
    if (node->isLeafNode() && CgHelper::isConjunction(node) && !CgHelper::isOnCycle(node)) {
      node->setState(CgNodeState::UNWIND_SAMPLE, 1);

      for (auto parentNode : node->getParentNodes()) {
        CgHelper::deleteInstrumentationIfRedundant(parentNode);
      }
    }
  }
}

//// RESET ESTIMATOR PHASE

ResetEstimatorPhase::ResetEstimatorPhase() : EstimatorPhase("Reset", true) {}

ResetEstimatorPhase::~ResetEstimatorPhase() {}

void ResetEstimatorPhase::modifyGraph(CgNodePtr mainMethod) {
  for (auto node : (*graph)) {
    node->reset();
  }
}

void ResetEstimatorPhase::printReport() {
  std::cout << "==" << report.phaseName << "== Phase " << std::endl << std::endl;
}

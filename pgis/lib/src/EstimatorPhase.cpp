/**
 * File: EstimatorPhase.cpp
 * License: Part of the metacg project. Licensed under BSD 3 clause license. See LICENSE.txt file at
 * https://github.com/tudasc/metacg/LICENSE.txt
 */

#include "EstimatorPhase.h"
#include "CgNodeMetaData.h"
#include <fstream>
#include <iomanip>  //  std::setw()
#include <iostream>

using namespace metacg;
using namespace pira;

#define NO_DEBUG

EstimatorPhase::EstimatorPhase(std::string name, bool isMetaPhase)
    : graph(nullptr),  // just so eclipse does not nag
      report(),        // initializes all members of report
      name(name),
      config(nullptr),
      noReportRequired(isMetaPhase) {}

void EstimatorPhase::generateReport() {
  for (auto node : (*graph)) {
    const auto bpd = node->get<BaseProfileData>();
    if (node->isInstrumented()) {
      /* Note: Instrumented call-paths do not count towards instrumented functions. */
      if (node->isInstrumentedCallpath()) {
        spdlog::get("console")->debug("Adding {} to the list of call path instrumentations", node->getFunctionName());
        report.instrumentedPaths.insert({node->getFunctionName(), node});
        continue;
      }

      report.instrumentedMethods += 1;
      report.instrumentedCalls += bpd->getNumberOfCalls();

      report.instrumentedNames.insert(node->getFunctionName());
      report.instrumentedNodes.insert(node);
    }

    // Edge instrumentation
    // XXX Is this still valid / required?
    for (CgNodePtr parent : node->getInstrumentedParentEdges()) {
      report.instrumentedEdges.insert({parent, node});
    }

    if (node->isUnwound()) {
      unsigned long long unwindSamples = 0;
      if (node->isUnwoundInstr()) {
        unwindSamples = bpd->getNumberOfCalls();
      } else {
        std::cerr << "Error in generateReport." << std::endl;
      }
      unsigned long long unwindSteps = 0;

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

  assert(report.instrumentedMethods >= report.instrumentedNames.size());
  assert(report.instrumentedMethods >= report.instrumentedNodes.size());
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

  for (auto node : nodesReachableFromMain) {
    if (graph->getGraph().find(node) != graph->getGraph().end()) {
      node->setReachable();
    }
  }

  // FIXME: Is this actually problematic?
  return;
#if false
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
#endif
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

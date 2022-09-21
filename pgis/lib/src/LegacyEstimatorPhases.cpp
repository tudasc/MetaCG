//
// Created by jp on 12.07.22.
//

#include "EstimatorPhase.h"
#include "MetaData/PGISMetaData.h"
#include <fstream>
#include <iostream>

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
//      node->setReachable();
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
      //      node->setState(CgNodeState::INSTRUMENT_WITNESS);
      pgis::instrumentNode(node);
    }
  }
}

//
// Created by jp on 12.07.22.
//

#include "EstimatorPhase.h"
#include "MetaData/PGISMetaData.h"
#include <fstream>
#include <iostream>

//// REMOVE UNRELATED NODES ESTIMATOR PHASE

RemoveUnrelatedNodesEstimatorPhase::RemoveUnrelatedNodesEstimatorPhase(metacg::Callgraph *cg,
                                                                       bool onlyRemoveUnrelatedNodes,
                                                                       bool aggressiveReduction)
    : EstimatorPhase("RemoveUnrelated", cg, true),
      numUnconnectedRemoved(0),
      numLeafsRemoved(0),
      numChainsRemoved(0),
      numAdvancedOptimizations(0),
      aggressiveReduction(aggressiveReduction),
      onlyRemoveUnrelatedNodes(onlyRemoveUnrelatedNodes) {}

RemoveUnrelatedNodesEstimatorPhase::~RemoveUnrelatedNodesEstimatorPhase() { nodesToRemove.clear(); }

void RemoveUnrelatedNodesEstimatorPhase::modifyGraph(metacg::CgNode *mainMethod) {
  /* remove unrelated nodes (not reachable from main) */
  CgNodeRawPtrUSet nodesReachableFromMain = CgHelper::getDescendants(mainMethod, graph);

  for (auto node : nodesReachableFromMain) {
    if (graph->hasNode(node)) {
      //      node->setReachable();
    }
  }
}

void RemoveUnrelatedNodesEstimatorPhase::checkLeafNodeForRemoval(metacg::CgNode *potentialLeaf) {
  if (CgHelper::isConjunction(potentialLeaf, graph)) {
    return;  // conjunctions are never removed
  }

  for (auto child : graph->getCallees(potentialLeaf)) {
    if (nodesToRemove.find(child) == nodesToRemove.end()) {
      return;
    }
  }

  nodesToRemove.insert(potentialLeaf);
  numLeafsRemoved++;

  for (auto parentNode : graph->getCallers(potentialLeaf)) {
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

WLInstrEstimatorPhase::WLInstrEstimatorPhase(const std::string &wlFilePath) : EstimatorPhase("WLInstr", nullptr) {
  std::ifstream ifStream(wlFilePath);
  if (!ifStream.good()) {
    std::cerr << "Error: can not find whitelist at .. " << wlFilePath << std::endl;
  }

  std::string buff;
  while (getline(ifStream, buff)) {
    whiteList.insert(std::hash<std::string>()(buff));
  }
}

void WLInstrEstimatorPhase::modifyGraph(metacg::CgNode *mainMethod) {
  for (const auto &elem : graph->getNodes()) {
    const auto &node = elem.second.get();
    if (whiteList.find(node->getId()) != whiteList.end()) {
      //      node->setState(CgNodeState::INSTRUMENT_WITNESS);
      pgis::instrumentNode(node);
    }
  }
}

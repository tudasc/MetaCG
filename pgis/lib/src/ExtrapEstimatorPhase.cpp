//
// Created by jp on 23.04.19.
//

#include "ExtrapEstimatorPhase.h"
#include "CgHelper.h"

#include "IPCGEstimatorPhase.h"

#include "EXTRAP_Function.hpp"
#include "EXTRAP_Model.hpp"

namespace pira {

void ExtrapLocalEstimatorPhaseBase::modifyGraph(CgNodePtr mainNode) {
  std::cout << "Running ExtrapLocalEstimatorPhaseBase::modifyGraph\n";
  for (const auto n : *graph) {
    if (shouldInstrument(n)) {
      n->setState(CgNodeState::INSTRUMENT_WITNESS);
      if (allNodesToMain) {
        auto nodesToMain = CgHelper::allNodesToMain(n, mainNode);
        std::cout << "Node " << n->getFunctionName() << " has " << nodesToMain.size() << " nodes on paths to main."
                  << std::endl;
        for (const auto ntm : nodesToMain) {
          ntm->setState(CgNodeState::INSTRUMENT_WITNESS);
        }
      }
    }
  }
}

bool ExtrapLocalEstimatorPhaseBase::shouldInstrument(CgNodePtr node) const {
  assert(false && "Base class should not be instantiated.");
  return false;
}

bool ExtrapLocalEstimatorPhaseSingleValueFilter::shouldInstrument(CgNodePtr node) const {
  if (!node->getExtrapModelConnector().isModelSet())
    return false;

  auto modelValue = node->getExtrapModelConnector().getEpolator().getExtrapolationValue();

  auto fVal = evalModelWValue(node, modelValue);

  std::cout << "Model value for " << node->getFunctionName() << " is calculated as: " << fVal << std::endl;

  return fVal > threshold;
}

void ExtrapLocalEstimatorPhaseSingleValueExpander::modifyGraph(CgNodePtr mainNode) {
  std::unordered_map<CgNodePtr, CgNodePtrUnorderedSet> pathsToMain;

  for (const auto n : *graph) {
    if (shouldInstrument(n)) {
      n->setState(CgNodeState::INSTRUMENT_WITNESS);
      if (allNodesToMain) {
        if (pathsToMain.find(n) == pathsToMain.end()) {
          auto nodesToMain = CgHelper::allNodesToMain(n, mainNode, pathsToMain);
          pathsToMain[n] = nodesToMain;
        }
        auto nodesToMain = pathsToMain[n];
        std::cout << "Found " << nodesToMain.size() << " nodes to main." << std::endl;
        for (const auto ntm : nodesToMain) {
          ntm->setState(CgNodeState::INSTRUMENT_WITNESS);
        }
      }
      std::cout << "Processing " << n->getChildNodes().size() << " child nodes" << std::endl;
      std::unordered_set<CgNodePtr> totalToMain;
      for (const auto c : n->getChildNodes()) {
        if (!c->getExtrapModelConnector().hasModels()) {
          // We use our heuristic to deepen the instrumentation
          StatementCountEstimatorPhase scep(200);  // we use some threshold value here?
          scep.estimateStatementCount(c);
          if (allNodesToMain) {
            if (pathsToMain.find(c) == pathsToMain.end()) {
              auto cLocal = c;
              auto nodesToMain = CgHelper::allNodesToMain(cLocal, mainNode, pathsToMain);
              pathsToMain[cLocal] = nodesToMain;
              totalToMain.insert(nodesToMain.begin(), nodesToMain.end());
              }
            }
          }
        }
      /*
      for (const auto ntm : totalToMain) {
        ntm->setState(CgNodeState::INSTRUMENT_WITNESS);
      }
      */
    }
  }
}

}  // namespace pira

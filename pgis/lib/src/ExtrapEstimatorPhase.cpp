//
// Created by jp on 23.04.19.
//

#include "ExtrapEstimatorPhase.h"
#include "CgHelper.h"

#include "IPCGEstimatorPhase.h"

#include "EXTRAP_Function.hpp"
#include "EXTRAP_Model.hpp"

#include <algorithm>

namespace pira {

void ExtrapLocalEstimatorPhaseBase::modifyGraph(CgNodePtr mainNode) {
  std::cout << "Running ExtrapLocalEstimatorPhaseBase::modifyGraph\n";
  for (const auto n : *graph) {
    auto [shouldInstr, funcRtVal] = shouldInstrument(n);
    if (shouldInstr) {
      n->setState(CgNodeState::INSTRUMENT_WITNESS);
      kernels.push_back({funcRtVal, n});

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

void ExtrapLocalEstimatorPhaseBase::printReport() {
  std::cout << "$$ Identified Kernels (w/ Runtime) $$\n";

  std::sort(std::begin(kernels), std::end(kernels), [&](auto &e1, auto &e2) { return e1.first > e2.first; });
  for (auto [t, k] : kernels) {
    std::cout << "- " << k->getFunctionName() << "\n  * Time: " << t << "\n";
    std::cout << "\t" << k->getFilename() << ":" << k->getLineNumber() << "\n\n";
  }

  std::cout << "$$ End Kernels $$" << std::endl;
}

std::pair<bool, double> ExtrapLocalEstimatorPhaseBase::shouldInstrument(CgNodePtr node) const {
  assert(false && "Base class should not be instantiated.");
  return {false, -1};
}

std::pair<bool, double> ExtrapLocalEstimatorPhaseSingleValueFilter::shouldInstrument(CgNodePtr node) const {
  if (!node->getExtrapModelConnector().isModelSet())
    return {false, -1};

  auto modelValue = node->getExtrapModelConnector().getEpolator().getExtrapolationValue();

  auto fVal = evalModelWValue(node, modelValue);

  std::cout << "Model value for " << node->getFunctionName() << " is calculated as: " << fVal << std::endl;

  return {fVal > threshold, fVal};
}

void ExtrapLocalEstimatorPhaseSingleValueExpander::modifyGraph(CgNodePtr mainNode) {
  std::unordered_map<CgNodePtr, CgNodePtrUnorderedSet> pathsToMain;

  for (const auto n : *graph) {
    auto [shouldInstr, funcRtVal] = shouldInstrument(n);
    if (shouldInstr) {
      n->setState(CgNodeState::INSTRUMENT_WITNESS);
      kernels.push_back({funcRtVal, n});

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

      std::unordered_set<CgNodePtr> totalToMain;
      for (const auto c : n->getChildNodes()) {
        if (!c->getExtrapModelConnector().hasModels()) {
          // We use our heuristic to deepen the instrumentation
          StatementCountEstimatorPhase scep(200);  // TODO we use some threshold value here?
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
    }
  }
}

}  // namespace pira

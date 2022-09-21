/**
 * File: ExtrapEstimatorPhase.cpp
 * License: Part of the MetaCG project. Licensed under BSD 3 clause license. See LICENSE.txt file at
 * https://github.com/tudasc/metacg/LICENSE.txt
 */

#include "ReachabilityAnalysis.h"

#include "ExtrapEstimatorPhase.h"
#include "CgHelper.h"
#include "config/GlobalConfig.h"
#include "config/ParameterConfig.h"

#include "IPCGEstimatorPhase.h"
#include "MetaData/PGISMetaData.h"

#include "EXTRAP_Function.hpp"
#include "EXTRAP_Model.hpp"

#include "spdlog/spdlog.h"

#include <algorithm>
#include <cassert>
#include <sstream>

using namespace metacg;

namespace pira {

void ExtrapLocalEstimatorPhaseBase::modifyGraph(CgNodePtr mainNode) {
  auto console = spdlog::get("console");
  console->trace("Running ExtrapLocalEstimatorPhaseBase::modifyGraph");
  metacg::analysis::ReachabilityAnalysis ra(graph);

  for (const auto &n : *graph) {
    auto [shouldInstr, funcRtVal] = shouldInstrument(n);
    if (shouldInstr) {
      auto useCSInstr =
          pgis::config::GlobalConfig::get().getAs<bool>(pgis::options::useCallSiteInstrumentation.cliName);
      if (useCSInstr && !n->get<PiraOneData>()->getHasBody()) {
        // If no definition, use call-site instrumentation
        pgis::instrumentPathNode(n);
//        n->setState(CgNodeState::INSTRUMENT_PATH);
      } else {
        pgis::instrumentNode(n);
//        n->setState(CgNodeState::INSTRUMENT_WITNESS);
      }
      kernels.emplace_back(funcRtVal, n);

      if (allNodesToMain) {
        auto nodesToMain = CgHelper::allNodesToMain(n, mainNode, ra);
        console->trace("Node {} has {} nodes on paths to main.", n->getFunctionName(), nodesToMain.size());
        for (const auto &ntm : nodesToMain) {
          pgis::instrumentNode(ntm);
//          ntm->setState(CgNodeState::INSTRUMENT_WITNESS);
        }
      }
    }
  }
}

void ExtrapLocalEstimatorPhaseBase::printReport() {
  auto console = spdlog::get("console");

  std::stringstream ss;

  std::sort(std::begin(kernels), std::end(kernels), [&](auto &e1, auto &e2) { return e1.first > e2.first; });
  for (auto [t, k] : kernels) {
    ss << "- " << k->getFunctionName() << "\n  * Time: " << t << "\n";
    auto [has, obj] = k->checkAndGet<pira::FilePropertiesMetaData>();
     if (has) {
       ss << "\t" << obj->origin << ':' << obj->lineNumber << "\n\n";
     }
  }

  console->info("$$ Identified Kernels (w/ Runtime) $$\n{}$$ End Kernels $$", ss.str());
}

std::pair<bool, double> ExtrapLocalEstimatorPhaseBase::shouldInstrument(CgNodePtr node) const {
  assert(false && "Base class should not be instantiated.");
  return {false, -1};
}

std::pair<bool, double> ExtrapLocalEstimatorPhaseSingleValueFilter::shouldInstrument(CgNodePtr node) const {
  spdlog::get("console")->trace("Running {}", __PRETTY_FUNCTION__);

  // get extrapolation threshold from parameter configPtr
  double extrapolationThreshold = pgis::config::ParameterConfig::get().getPiraIIConfig()->extrapolationThreshold;

  if (useRuntimeOnly) {
    auto pdII = node->get<PiraTwoData>();
    auto rtVec = pdII->getRuntimeVec();

    const auto median = [&](auto vec) {
      spdlog::get("console")->debug("Vec size {}", vec.size());
      if (vec.size() % 2 == 0) {
        return vec[vec.size() / 2];
      } else {
        return vec[vec.size() / 2 + 1];
      }
    };

    if (rtVec.size() == 0) {
      return {false, .0};
    }

    auto medianValue = median(rtVec);
    std::string funcName{__PRETTY_FUNCTION__};
    spdlog::get("console")->debug("{}: No. of RT values: {}, median RT value {}, threshold value {}", funcName,
                                  rtVec.size(), medianValue, extrapolationThreshold);
    if (medianValue > extrapolationThreshold) {
      return {true, medianValue};
    }
  }

  if (!node->get<PiraTwoData>()->getExtrapModelConnector().isModelSet()) {
    spdlog::get("console")->trace("Model not set for {}", node->getFunctionName());
    return {false, -1};
  }

  auto modelValue = node->get<PiraTwoData>()->getExtrapModelConnector().getEpolator().getExtrapolationValue();

  auto fVal = evalModelWValue(node, modelValue);

  spdlog::get("console")->debug("Model value for function {} is calcuated at x = {} as {}", node->getFunctionName(),
                                modelValue[0].second, fVal);

  return {fVal > extrapolationThreshold, fVal};
}

void ExtrapLocalEstimatorPhaseSingleValueExpander::modifyGraph(CgNodePtr mainNode) {
  std::unordered_map<CgNodePtr, CgNodePtrUnorderedSet> pathsToMain;
  metacg::analysis::ReachabilityAnalysis ra(graph);

  // get statement threshold from parameter configPtr
  int statementThreshold = pgis::config::ParameterConfig::get().getPiraIIConfig()->statementThreshold;

  for (const auto &n : *graph) {
    auto console = spdlog::get("console");
    console->trace("Running ExtrapLocalEstimatorPhaseExpander::modifyGraph on {}", n->getFunctionName());
    auto [shouldInstr, funcRtVal] = shouldInstrument(n);
    if (shouldInstr) {
      if (!n->get<PiraOneData>()->getHasBody() && n->get<BaseProfileData>()->getRuntimeInSeconds() == .0) {
        // If no definition, use call-site instrumentation
//        n->setState(CgNodeState::INSTRUMENT_PATH);
        pgis::instrumentPathNode(n);
      } else {
//        n->setState(CgNodeState::INSTRUMENT_WITNESS);
        pgis::instrumentNode(n);
      }
      kernels.emplace_back(funcRtVal, n);

      if (allNodesToMain) {
        if (pathsToMain.find(n) == pathsToMain.end()) {
          auto nodesToMain = CgHelper::allNodesToMain(n, mainNode, pathsToMain, ra);
          pathsToMain[n] = nodesToMain;
        }
        auto nodesToMain = pathsToMain[n];
        spdlog::get("console")->trace("Found {} nodes to main.", nodesToMain.size());
        for (const auto &ntm : nodesToMain) {
//          ntm->setState(CgNodeState::INSTRUMENT_WITNESS);
          pgis::instrumentNode(ntm);
        }
      }

      std::unordered_set<CgNodePtr> totalToMain;
      for (const auto &c : n->getChildNodes()) {
        if (!c->get<PiraTwoData>()->getExtrapModelConnector().hasModels()) {
          // We use our heuristic to deepen the instrumentation
          StatementCountEstimatorPhase scep(statementThreshold);  // TODO we use some threshold value here?
          scep.estimateStatementCount(c, ra);
          if (allNodesToMain) {
            if (pathsToMain.find(c) == pathsToMain.end()) {
              auto cLocal = c;
              auto nodesToMain = CgHelper::allNodesToMain(cLocal, mainNode, pathsToMain, ra);
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

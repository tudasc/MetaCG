/**
 * File: ExtrapEstimatorPhase.cpp
 * License: Part of the MetaCG project. Licensed under BSD 3 clause license. See LICENSE.txt file at
 * https://github.com/tudasc/metacg/LICENSE.txt
 */

#include "ReachabilityAnalysis.h"

#include "CgHelper.h"
#include "ExtrapEstimatorPhase.h"
#include "config/GlobalConfig.h"
#include "config/ParameterConfig.h"

#include "IPCGEstimatorPhase.h"
#include "MetaData/PGISMetaData.h"

#include "EXTRAP_Function.hpp"
#include "EXTRAP_Model.hpp"

#include <algorithm>
#include <cassert>
#include <sstream>

using namespace metacg;

namespace pira {

void ExtrapLocalEstimatorPhaseBase::modifyGraph(metacg::CgNode* mainNode) {
  auto console = metacg::MCGLogger::instance().getConsole();
  console->trace("Running ExtrapLocalEstimatorPhaseBase::modifyGraph");
  metacg::analysis::ReachabilityAnalysis ra(graph);

  for (const auto& elem : graph->getNodes()) {
    const auto& n = elem.second.get();
    auto [shouldInstr, funcRtVal] = shouldInstrument(n);
    if (shouldInstr) {
      auto useCSInstr =
          pgis::config::GlobalConfig::get().getAs<bool>(pgis::options::useCallSiteInstrumentation.cliName);
      if (useCSInstr && !n->getOrCreateMD<PiraOneData>()->getHasBody()) {
        // If no definition, use call-site instrumentation
        metacg::pgis::instrumentPathNode(n);
        //        n->setState(CgNodeState::INSTRUMENT_PATH);
      } else {
        metacg::pgis::instrumentNode(n);
        //        n->setState(CgNodeState::INSTRUMENT_WITNESS);
      }
      kernels.emplace_back(funcRtVal, n);

      if (allNodesToMain) {
        auto nodesToMain = CgHelper::allNodesToMain(n, mainNode, graph, ra);
        console->trace("Node {} has {} nodes on paths to main.", n->getFunctionName(), nodesToMain.size());
        for (const auto& ntm : nodesToMain) {
          metacg::pgis::instrumentNode(ntm);
          //          ntm->setState(CgNodeState::INSTRUMENT_WITNESS);
        }
      }
    }
  }
}

void ExtrapLocalEstimatorPhaseBase::printReport() {
  auto console = metacg::MCGLogger::instance().getConsole();

  std::stringstream ss;

  std::sort(std::begin(kernels), std::end(kernels), [&](auto& e1, auto& e2) { return e1.first > e2.first; });
  for (auto [t, k] : kernels) {
    ss << "- " << k->getFunctionName() << "\n  * Time: " << t << "\n";
    auto [has, obj] = k->checkAndGet<pira::FilePropertiesMetaData>();
    if (has) {
      ss << "\t" << obj->origin << ':' << obj->lineNumber << "\n\n";
    }
  }

  console->info("$$ Identified Kernels (w/ Runtime) $$\n{}$$ End Kernels $$", ss.str());
}

std::pair<bool, double> ExtrapLocalEstimatorPhaseBase::shouldInstrument(metacg::CgNode* node) const {
  assert(false && "Base class should not be instantiated.");
  return {false, -1};
}

std::pair<bool, double> ExtrapLocalEstimatorPhaseSingleValueFilter::shouldInstrument(metacg::CgNode* node) const {
  auto console = metacg::MCGLogger::instance().getConsole();
  console->trace("Running {}", __PRETTY_FUNCTION__);

  // get extrapolation threshold from parameter configPtr
  const double extrapolationThreshold = pgis::config::ParameterConfig::get().getPiraIIConfig()->extrapolationThreshold;

  if (useRuntimeOnly) {
    auto pdII = node->get<PiraTwoData>();
    auto rtVec = pdII->getRuntimeVec();

    const auto median = [&](auto vec) {
      console->debug("Vec size {}", vec.size());
      if (vec.size() % 2 == 0) {
        return vec[vec.size() / 2];
      } else {
        return vec[vec.size() / 2 + 1];
      }
    };

    if (rtVec.empty()) {
      return {false, .0};
    }

    auto medianValue = median(rtVec);
    const std::string funcName{__PRETTY_FUNCTION__};
    console->debug("{}: No. of RT values: {}, median RT value {}, threshold value {}", funcName, rtVec.size(),
                   medianValue, extrapolationThreshold);
    if (medianValue > extrapolationThreshold) {
      return {true, medianValue};
    }
  }

  if (!node->get<PiraTwoData>()->getExtrapModelConnector().isModelSet()) {
    console->trace("Model not set for {}", node->getFunctionName());
    return {false, -1};
  }

  auto modelValue = node->get<PiraTwoData>()->getExtrapModelConnector().getEpolator().getExtrapolationValue();

  auto fVal = evalModelWValue(node, modelValue);

  console->debug("Model value for function {} is calcuated at x = {} as {}", node->getFunctionName(),
                 modelValue[0].second, fVal);

  return {fVal > extrapolationThreshold, fVal};
}

void ExtrapLocalEstimatorPhaseSingleValueExpander::modifyGraph(metacg::CgNode* mainNode) {
  std::unordered_map<metacg::CgNode*, CgNodeRawPtrUSet> pathsToMain;
  metacg::analysis::ReachabilityAnalysis ra(graph);

  // get statement threshold from parameter configPtr
  const int statementThreshold = pgis::config::ParameterConfig::get().getPiraIIConfig()->statementThreshold;

  for (const auto& elem : graph->getNodes()) {
    const auto& n = elem.second.get();
    auto console = metacg::MCGLogger::instance().getConsole();
    console->trace("Running ExtrapLocalEstimatorPhaseExpander::modifyGraph on {}", n->getFunctionName());
    auto [shouldInstr, funcRtVal] = shouldInstrument(n);
    if (shouldInstr) {
      if (!n->getOrCreateMD<PiraOneData>()->getHasBody() && n->get<BaseProfileData>()->getRuntimeInSeconds() == .0) {
        // If no definition, use call-site instrumentation
        metacg::pgis::instrumentPathNode(n);
      } else {
        metacg::pgis::instrumentNode(n);
      }
      kernels.emplace_back(funcRtVal, n);

      if (allNodesToMain) {
        if (pathsToMain.find(n) == pathsToMain.end()) {
          auto nodesToMain = CgHelper::allNodesToMain(n, mainNode, graph, pathsToMain, ra);
          pathsToMain[n] = nodesToMain;
        }
        auto nodesToMain = pathsToMain[n];
        console->trace("Found {} nodes to main.", nodesToMain.size());
        for (const auto& ntm : nodesToMain) {
          metacg::pgis::instrumentNode(ntm);
        }
      }

      std::unordered_set<metacg::CgNode*> totalToMain;
      for (const auto& c : graph->getCallees(n->getId())) {
        if (!c->get<PiraTwoData>()->getExtrapModelConnector().hasModels()) {
          // We use our heuristic to deepen the instrumentation
          StatementCountEstimatorPhase scep(statementThreshold, graph);  // TODO we use some threshold value here?
          scep.estimateStatementCount(c, ra);
          if (allNodesToMain) {
            if (pathsToMain.find(c) == pathsToMain.end()) {
              auto cLocal = c;
              auto nodesToMain = CgHelper::allNodesToMain(cLocal, mainNode, graph, pathsToMain, ra);
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

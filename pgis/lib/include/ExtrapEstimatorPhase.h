/**
 * File: ExtrapEstimatorPhase.h
 * License: Part of the metacg project. Licensed under BSD 3 clause license. See LICENSE.txt file at
 * https://github.com/tudasc/metacg/LICENSE.txt
 */

//
// Created by jp on 23.04.19.
//

#ifndef PGOE_EXTRAPESTIMATORPHASE_H
#define PGOE_EXTRAPESTIMATORPHASE_H

#include "EstimatorPhase.h"
#include "LoggerUtil.h"
#include "MetaData/CgNodeMetaData.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wextra"
#pragma GCC diagnostic ignored "-Wpedantic"
#pragma GCC diagnostic ignored "-Wunused-variable"
#pragma GCC diagnostic ignored "-Wsign-compare"
#include "EXTRAP_MultiParameterFunction.hpp"
#include "EXTRAP_SingleParameterFunction.hpp"
#pragma GCC diagnostic pop

namespace pira {

template <typename ContT1, typename ContT2>
auto zip_to_map(ContT1& keys, ContT2& vals) {
  auto k_it = std::begin(keys);
  auto v_it = std::begin(vals);
  std::map<typename ContT1::value_type, typename ContT2::value_type> v_map;
  for (; k_it != std::end(keys) && v_it != std::end(vals); k_it++, v_it++) {
    v_map[*k_it] = *v_it;
  }
  return v_map;
}

inline bool isConstant(EXTRAP::MultiParameterFunction* func) { return func->getMultiParameterTerms().size() == 0; }

inline bool isConstant(EXTRAP::SingleParameterFunction* func) { return func->getCompoundTerms().size() == 0; }

template <typename T>
inline bool isConstant(T paramCont, const std::unique_ptr<EXTRAP::Function>& func) {
  if (paramCont.size() < 1) {
    metacg::MCGLogger::instance().getErrConsole()->error("{}: Less than one parameter found.", __FUNCTION__);
    assert(paramCont.size() > 0 && "There should be at least one parameter stored per function");
  }

  if (paramCont.size() == 1) {
    return isConstant(static_cast<EXTRAP::SingleParameterFunction*>(func.get()));
  } else {
    return isConstant(static_cast<EXTRAP::MultiParameterFunction*>(func.get()));
  }
}

class ExtrapLocalEstimatorPhaseBase : public EstimatorPhase {
 public:
  using value_type = EXTRAP::Value;

  ExtrapLocalEstimatorPhaseBase(metacg::Callgraph* cg, bool allNodesToMain = false, bool useRuntimeOnly = false)
      : EstimatorPhase("ExtrapLocalEstimatorPhaseBase", cg),
        allNodesToMain(allNodesToMain),
        useRuntimeOnly(useRuntimeOnly) {}

  virtual ~ExtrapLocalEstimatorPhaseBase() = default;

  ExtrapLocalEstimatorPhaseBase(ExtrapLocalEstimatorPhaseBase& other) = default;

  ExtrapLocalEstimatorPhaseBase(ExtrapLocalEstimatorPhaseBase&& other) = default;

  ExtrapLocalEstimatorPhaseBase& operator=(ExtrapLocalEstimatorPhaseBase& other) = default;

  ExtrapLocalEstimatorPhaseBase& operator=(ExtrapLocalEstimatorPhaseBase&& other) = default;

  virtual void modifyGraph(metacg::CgNode* mainNode) override;

  virtual std::pair<bool, double> shouldInstrument(metacg::CgNode* node) const;

  void printReport() override;

 protected:
  /*
   * For now no type check, these all need to be of EXTRAP::Value
   */
  /*
  template <typename... Vals>
  value_type evalModelWValue(metacg::CgNode* n, Vals... values) const;
  */
  auto evalModelWValue(metacg::CgNode* n, std::vector<std::pair<std::string, double>> values) const;
  bool allNodesToMain;
  bool useRuntimeOnly;

  std::vector<std::pair<double, metacg::CgNode*>> kernels;
};

#if 0
template <typename... Vals>
ExtrapLocalEstimatorPhaseBase::value_type ExtrapLocalEstimatorPhaseBase::evalModelWValue(metacg::CgNode* n,
                                                                                         Vals... values) const {
  auto fModel = n->getExtrapModelConnector().getEPModelFunction();

  std::vector<double> vals{values...};
  auto keys = n->getExtrapModelConnector().getParamList();

  auto plist = zip_to_map(keys, vals);

  auto fVal = fModel->evaluate(plist);
  return fVal;
}
#endif

auto ExtrapLocalEstimatorPhaseBase::evalModelWValue(metacg::CgNode* n,
                                                    std::vector<std::pair<std::string, double>> values) const {
  auto& fModel = n->get<PiraTwoData>()->getExtrapModelConnector().getEPModelFunction();

  std::map<EXTRAP::Parameter, double> evalOps;

  auto console = metacg::MCGLogger::instance().getConsole();
  for (const auto& p : values) {
    console->debug("EvalModel: Setting {} to {}", p.first, p.second);
    evalOps.insert(std::make_pair(EXTRAP::Parameter(p.first), p.second));
  }

  if (false && isConstant(evalOps, fModel)) {
    return -1.0;
  }

  auto fVal = fModel->evaluate(evalOps);

  return fVal;
}

class ExtrapLocalEstimatorPhaseSingleValueFilter : public ExtrapLocalEstimatorPhaseBase {
 public:
  ExtrapLocalEstimatorPhaseSingleValueFilter(metacg::Callgraph* cg, bool allNodesToMain = false,
                                             bool useRuntimeOnly = false)
      : ExtrapLocalEstimatorPhaseBase(cg, allNodesToMain, useRuntimeOnly) {}
  virtual std::pair<bool, double> shouldInstrument(metacg::CgNode* node) const override;
};

class ExtrapLocalEstimatorPhaseSingleValueExpander : public ExtrapLocalEstimatorPhaseSingleValueFilter {
 public:
  ExtrapLocalEstimatorPhaseSingleValueExpander(metacg::Callgraph* cg, bool allNodesToMain = true,
                                               bool useRuntimeOnly = false)
      : ExtrapLocalEstimatorPhaseSingleValueFilter(cg, allNodesToMain, useRuntimeOnly) {}

  virtual void modifyGraph(metacg::CgNode* mainNode) override;
};

}  // namespace pira
#endif  // PGOE_EXTRAPESTIMATORPHASE_H

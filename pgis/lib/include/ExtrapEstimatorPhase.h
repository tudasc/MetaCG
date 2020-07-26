//
// Created by jp on 23.04.19.
//

#ifndef PGOE_EXTRAPESTIMATORPHASE_H
#define PGOE_EXTRAPESTIMATORPHASE_H

#include "EstimatorPhase.h"

#include "spdlog/spdlog.h"

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
auto zip_to_map(ContT1 &keys, ContT2 &vals) {
  auto k_it = std::begin(keys);
  auto v_it = std::begin(vals);
  std::map<typename ContT1::value_type, typename ContT2::value_type> v_map;
  for (; k_it != std::end(keys) && v_it != std::end(vals); k_it++, v_it++) {
    v_map[*k_it] = *v_it;
  }
  return v_map;
}

inline bool isConstant(EXTRAP::MultiParameterFunction *func) { return func->getMultiParameterTerms().size() == 0; }

inline bool isConstant(EXTRAP::SingleParameterFunction *func) { return func->getCompoundTerms().size() == 0; }

template <typename T>
inline bool isConstant(T paramCont, EXTRAP::Function *func) {
  if (paramCont.size() < 1) {
    spdlog::get("errconsole")->error("{}: Less than one parameter found.", __FUNCTION__);
    assert(paramCont.size() > 0 && "There should be at least one parameter stored per function");
  }

  if (paramCont.size() == 1) {
    return isConstant(static_cast<EXTRAP::SingleParameterFunction *>(func));
  } else {
    return isConstant(static_cast<EXTRAP::MultiParameterFunction *>(func));
  }
}

class ExtrapLocalEstimatorPhaseBase : public EstimatorPhase {
 public:
  using value_type = EXTRAP::Value;

  ExtrapLocalEstimatorPhaseBase(bool allNodesToMain = false, bool useRuntimeOnly = false)
      : EstimatorPhase("ExtrapLocalEstimatorPhaseBase"), allNodesToMain(allNodesToMain), useRuntimeOnly(useRuntimeOnly) {}

  virtual ~ExtrapLocalEstimatorPhaseBase() = default;

  ExtrapLocalEstimatorPhaseBase(ExtrapLocalEstimatorPhaseBase &other) = default;

  ExtrapLocalEstimatorPhaseBase(ExtrapLocalEstimatorPhaseBase &&other) = default;

  ExtrapLocalEstimatorPhaseBase &operator=(ExtrapLocalEstimatorPhaseBase &other) = default;

  ExtrapLocalEstimatorPhaseBase &operator=(ExtrapLocalEstimatorPhaseBase &&other) = default;

  virtual void modifyGraph(CgNodePtr mainNode) override;

  virtual std::pair<bool, double> shouldInstrument(CgNodePtr node) const;

  void printReport() override;

 protected:
  /*
   * For now no type check, these all need to be of EXTRAP::Value
   */
  /*
  template <typename... Vals>
  value_type evalModelWValue(CgNodePtr n, Vals... values) const;
  */
  auto evalModelWValue(CgNodePtr n, std::vector<std::pair<std::string, double>> values) const;
  bool allNodesToMain;
  bool useRuntimeOnly;

  std::vector<std::pair<double, CgNodePtr>> kernels;
};

#if 0
template <typename... Vals>
ExtrapLocalEstimatorPhaseBase::value_type ExtrapLocalEstimatorPhaseBase::evalModelWValue(CgNodePtr n,
                                                                                         Vals... values) const {
  auto fModel = n->getExtrapModelConnector().getEPModelFunction();

  std::vector<double> vals{values...};
  auto keys = n->getExtrapModelConnector().getParamList();

  auto plist = zip_to_map(keys, vals);

  auto fVal = fModel->evaluate(plist);
  return fVal;
}
#endif

auto ExtrapLocalEstimatorPhaseBase::evalModelWValue(CgNodePtr n,
                                                    std::vector<std::pair<std::string, double>> values) const {
  auto fModel = n->get<PiraTwoData>()->getExtrapModelConnector().getEPModelFunction();

  std::map<EXTRAP::Parameter, double> evalOps;

  auto console = spdlog::get("console");
  for (const auto &p : values) {
    console->trace("Setting {} to {}", p.first, p.second);
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
  ExtrapLocalEstimatorPhaseSingleValueFilter(double threshold = 1.0, bool allNodesToMain = false, bool useRuntimeOnly = false)
      : ExtrapLocalEstimatorPhaseBase(allNodesToMain, useRuntimeOnly), threshold(threshold) {}
  virtual std::pair<bool, double> shouldInstrument(CgNodePtr node) const override;

 private:
  double threshold;
};

class ExtrapLocalEstimatorPhaseSingleValueExpander : public ExtrapLocalEstimatorPhaseSingleValueFilter {
 public:
  ExtrapLocalEstimatorPhaseSingleValueExpander(double threshold = 1.0, bool allNodesToMain = true, bool useRuntimeOnly = false)
      : ExtrapLocalEstimatorPhaseSingleValueFilter(threshold, allNodesToMain, useRuntimeOnly) {}

  virtual void modifyGraph(CgNodePtr mainNode) override;
};

}  // namespace pira
#endif  // PGOE_EXTRAPESTIMATORPHASE_H

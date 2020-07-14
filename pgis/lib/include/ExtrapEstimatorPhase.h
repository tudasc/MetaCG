//
// Created by jp on 23.04.19.
//

#ifndef PGOE_EXTRAPESTIMATORPHASE_H
#define PGOE_EXTRAPESTIMATORPHASE_H

#include "EstimatorPhase.h"

#include "EXTRAP_MultiParameterFunction.hpp"
#include "EXTRAP_SingleParameterFunction.hpp"

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


inline bool isConstant(EXTRAP::MultiParameterFunction *func) {
  return func->getMultiParameterTerms().size() == 0;
}

inline bool isConstant(EXTRAP::SingleParameterFunction *func) {
  return func->getCompoundTerms().size() == 0;
}

template<typename T>
inline bool isConstant(T paramCont, EXTRAP::Function *func) {
  if (paramCont.size() < 1) {
    std::cerr << __FUNCTION__ << ": Less than one parameter found." << std::endl;
    assert(paramCont.size() > 0 && "There should be at least one parameter stored per function");
  }

  if (paramCont.size() == 1) {
    return isConstant(static_cast<EXTRAP::SingleParameterFunction*>(func));
  } else {
    return isConstant(static_cast<EXTRAP::MultiParameterFunction*>(func));
  }
}

class ExtrapLocalEstimatorPhaseBase : public EstimatorPhase {
 public:
  using value_type = EXTRAP::Value;

  ExtrapLocalEstimatorPhaseBase(bool allNodesToMain = false)
      : EstimatorPhase("ExtrapLocalEstimatorPhaseBase"), allNodesToMain(allNodesToMain) {}

  virtual ~ExtrapLocalEstimatorPhaseBase() = default;

  ExtrapLocalEstimatorPhaseBase(ExtrapLocalEstimatorPhaseBase &other) = default;

  ExtrapLocalEstimatorPhaseBase(ExtrapLocalEstimatorPhaseBase &&other) = default;

  ExtrapLocalEstimatorPhaseBase &operator=(ExtrapLocalEstimatorPhaseBase &other) = default;

  ExtrapLocalEstimatorPhaseBase &operator=(ExtrapLocalEstimatorPhaseBase &&other) = default;

  virtual void modifyGraph(CgNodePtr mainNode) override;

  virtual bool shouldInstrument(CgNodePtr node) const;

 protected:
  /*
   * For now no type check, these all need to be of EXTRAP::Value
   */
  /*
  template <typename... Vals>
  value_type evalModelWValue(CgNodePtr n, Vals... values) const;
  */
  auto evalModelWValue(CgNodePtr n, std::vector<std::pair<std::string, double> > values) const;
  bool allNodesToMain;
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

auto ExtrapLocalEstimatorPhaseBase::evalModelWValue(CgNodePtr n, std::vector<std::pair<std::string, double> > values) const {
  auto fModel = n->getExtrapModelConnector().getEPModelFunction();

  std::map<EXTRAP::Parameter, double> evalOps;

  for (const auto &p : values) {
    std::cout << "Setting: " << p.first << " = " << p.second << std::endl;
    evalOps.insert(std::make_pair(EXTRAP::Parameter(p.first), p.second));
  }
  
  //if (isConstant(evalOps, fModel)) {
  //  return -1.0;
  //}

  auto fVal = fModel->evaluate(evalOps);

  return fVal;
}

class ExtrapLocalEstimatorPhaseSingleValueFilter : public ExtrapLocalEstimatorPhaseBase {
 public:
  ExtrapLocalEstimatorPhaseSingleValueFilter(double threshold = 1.0, bool allNodesToMain = false)
      : ExtrapLocalEstimatorPhaseBase(allNodesToMain), threshold(threshold) {}
  virtual bool shouldInstrument(CgNodePtr node) const override;

 private:
  double threshold;
};

class ExtrapLocalEstimatorPhaseSingleValueExpander : public ExtrapLocalEstimatorPhaseSingleValueFilter {
 public:
  ExtrapLocalEstimatorPhaseSingleValueExpander(double threshold = 1.0, bool allNodesToMain = true)
      : ExtrapLocalEstimatorPhaseSingleValueFilter(threshold, allNodesToMain) {}

  virtual void modifyGraph(CgNodePtr mainNode) override;
};

}  // namespace pira
#endif  // PGOE_EXTRAPESTIMATORPHASE_H

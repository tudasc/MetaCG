#ifndef LI_ONLYMAINESTIMATORPHASE_H
#define LI_ONLYMAINESTIMATORPHASE_H

#include <EstimatorPhase.h>

namespace LoadImbalance {
/**
 * Trivial estimator phase which is used in the first iteration of load imbalance detection
 */
class OnlyMainEstimatorPhase : public EstimatorPhase {
 public:
  OnlyMainEstimatorPhase();
  ~OnlyMainEstimatorPhase();

  void modifyGraph(CgNodePtr mainMethod) override;
};
}  // namespace LoadImbalance

#endif
/**
 * File: OnlyMainEstimatorPhase.h
 * License: Part of the metacg project. Licensed under BSD 3 clause license. See LICENSE.txt file at
 * https://github.com/tudasc/metacg/LICENSE.txt
 */

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
  ~OnlyMainEstimatorPhase() noexcept;

  void modifyGraph(metacg::CgNode* mainMethod) override;
};
}  // namespace LoadImbalance

#endif

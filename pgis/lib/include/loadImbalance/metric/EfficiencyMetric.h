/**
 * File: EfficiencyMetric.h
 * License: Part of the MetaCG project. Licensed under BSD 3 clause license. See LICENSE.txt file at
 * https://github.com/tudasc/metacg/LICENSE.txt
 */

#ifndef LI_EFFICIENCY_METRIC_H
#define LI_EFFICIENCY_METRIC_H

#include "AbstractMetric.h"

namespace LoadImbalance {

/**
 * Load imbalance metric "Efficiency"
 */
class EfficiencyMetric : public AbstractMetric {
 public:
  EfficiencyMetric();
  ~EfficiencyMetric() = default;

  double calc() const override { return this->mean != 0. ? max / mean : 0.; };
};

EfficiencyMetric::EfficiencyMetric() : AbstractMetric() {}

}  // namespace LoadImbalance

#endif

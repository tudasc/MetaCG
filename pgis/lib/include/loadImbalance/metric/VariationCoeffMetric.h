/**
 * File: VariationCoeffMetric.h
 * License: Part of the metacg project. Licensed under BSD 3 clause license. See LICENSE.txt file at
 * https://github.com/tudasc/metacg/LICENSE.txt
 */

#ifndef LI_VARIATIONCOEFF_METRIC_H
#define LI_VARIATIONCOEFF_METRIC_H

#include "AbstractMetric.h"

namespace LoadImbalance {
/**
 * Load imbalance metric: Variation Coefficient
 */
class VariationCoeffMetric : public AbstractMetric {
 public:
  VariationCoeffMetric();
  ~VariationCoeffMetric() = default;

  double calc() const override { return this->mean != 0 ? this->stdev / this->mean : 0.; }
};

VariationCoeffMetric::VariationCoeffMetric() : AbstractMetric() {}

}  // namespace LoadImbalance

#endif

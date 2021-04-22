/**
 * File: ImbalancePercentageMetric.h
 * License: Part of the MetaCG project. Licensed under BSD 3 clause license. See LICENSE.txt file at https://github.com/tudasc/metacg/LICENSE.txt
 */

#ifndef LI_IMBALANCEPERCENTAGE_METRIC_H
#define LI_IMBALANCEPERCENTAGE_METRIC_H

#include "AbstractMetric.h"

namespace LoadImbalance {

/**
 * Load imbalance metric: "Imbalance Percentage"
 */
class ImbalancePercentageMetric : public AbstractMetric {
 public:
  ImbalancePercentageMetric();
  ~ImbalancePercentageMetric() = default;

  double calc() const override { return count > 1 ? (max - mean) / max * count / (count - 1) : 0.0; };
};

ImbalancePercentageMetric::ImbalancePercentageMetric() : AbstractMetric() {}

}  // namespace LoadImbalance

#endif

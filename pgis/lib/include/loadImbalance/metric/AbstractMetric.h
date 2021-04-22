/**
 * File: AbstractMetric.h
 * License: Part of the MetaCG project. Licensed under BSD 3 clause license. See LICENSE.txt file at https://github.com/tudasc/metacg/LICENSE.txt
 */

#ifndef LI_ABSTRACT_METRIC_H
#define LI_ABSTRACT_METRIC_H

#include <CgNode.h>
#include <sstream>

namespace LoadImbalance {

/**
 * Base class for load imbalance metrics.
 * Provides standard interface which is used by LIEstimatorPhase.
 * Calculates a series of standard statistical indicators which then can be used by subclasses.
 */
class AbstractMetric {
 public:
  AbstractMetric();
  virtual ~AbstractMetric() = default;

  void setNode(CgNodePtr newNode, std::ostringstream &debugString);
  void setNode(CgNodePtr newNode);

  /**
   * calculate metric for node which has been last set by setNode
   */
  virtual double calc() const = 0;

 protected:
  CgNodePtr node;

  double max;
  double min;
  double mean;
  double stdev;

  // number of non-empty locations
  int count;

 private:
  void calcIndicators(std::ostringstream &debugString);
};
}  // namespace LoadImbalance

#endif

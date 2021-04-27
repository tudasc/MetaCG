/**
 * File: LIEstimatorPhase.h
 * License: Part of the MetaCG project. Licensed under BSD 3 clause license. See LICENSE.txt file at
 * https://github.com/tudasc/metacg/LICENSE.txt
 */

#ifndef LI_ESTIMATORPHASE_H
#define LI_ESTIMATORPHASE_H

#include "Config.h"
#include "EstimatorPhase.h"
#include "loadImbalance/metric/AbstractMetric.h"

#include <map>
#include <queue>
#include <set>

namespace LoadImbalance {

/**
 * EstimatorPhase implementing the "iterative descent" in every PIRA iteration
 */
class LIEstimatorPhase : public EstimatorPhase {
 public:
  LIEstimatorPhase(Config config);
  ~LIEstimatorPhase() override;

  void modifyGraph(CgNodePtr mainMethod) override;

 private:
  AbstractMetric *metric;

  /**
   * Config used for this run of load imbalance detection
   */
  Config c;

  // utility functions
  // =================
  /**
   * Instrument all children which have not been marked as irrelevant
   */
  void instrumentRelevantChildren(CgNodePtr node, pira::Statements statementThreshold, std::ostringstream &debugString);

  void contextHandling(CgNodePtr n, CgNodePtr mainNod, std::ostringstream &debugString);

  /**
   * check whether there is a path from start to end with steps as maximum length
   */
  static bool reachableInNSteps(CgNodePtr start, CgNodePtr end, int steps);

  /**
   * mark a node for instrumentation
   */
  static void instrument(CgNodePtr node);
};
}  // namespace LoadImbalance

#endif

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

  void contextHandling(CgNodePtr n, CgNodePtr mainNode);

  /**
   * check whether there is a path from start to end with steps as maximum length
   */
  static bool reachableInNSteps(CgNodePtr start, CgNodePtr end, int steps);

  void findSyncPoints(CgNodePtr node);

  /**
   * mark a node for instrumentation
   */
  static void instrument(CgNodePtr node);

  /**
   * Instrument all descendants of start node if they correspond the a pattern
   */
  void instrumentByPattern(CgNodePtr startNode, std::function< bool(CgNodePtr) > pattern, std::ostringstream& debugString);
};
}  // namespace LoadImbalance

#endif

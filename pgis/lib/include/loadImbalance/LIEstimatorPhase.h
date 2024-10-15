/**
 * File: LIEstimatorPhase.h
 * License: Part of the metacg project. Licensed under BSD 3 clause license. See LICENSE.txt file at
 * https://github.com/tudasc/metacg/LICENSE.txt
 */

#ifndef LI_ESTIMATORPHASE_H
#define LI_ESTIMATORPHASE_H

#include "EstimatorPhase.h"
#include "LIConfig.h"
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
  explicit LIEstimatorPhase(std::unique_ptr<LIConfig>&& config, metacg::Callgraph* cg);
  ~LIEstimatorPhase() override;

  void modifyGraph(metacg::CgNode* mainMethod) override;

  void doPrerequisites() override { CgHelper::calculateInclusiveStatementCounts(graph->getMain(), graph); }

 private:
  AbstractMetric* metric;

  /**
   * LIConfig used for this run of load imbalance detection
   */
  std::unique_ptr<LIConfig> c;

  // utility functions
  // =================
  /**
   * Instrument all children which have not been marked as irrelevant
   */
  void instrumentRelevantChildren(metacg::CgNode* node, pira::Statements statementThreshold,
                                  std::ostringstream& debugString);

  void contextHandling(metacg::CgNode* n, metacg::CgNode* mainNode, metacg::analysis::ReachabilityAnalysis& ra);

  /**
   * check whether there is a path from start to end with steps as maximum length
   */
  bool reachableInNSteps(metacg::CgNode* start, metacg::CgNode* end, int steps);

  void findSyncPoints(metacg::CgNode* node);

  /**
   * mark a node for instrumentation
   */
  static void instrument(metacg::CgNode* node);

  /**
   * Instrument all descendants of start node if they correspond the a pattern
   */
  void instrumentByPattern(metacg::CgNode* startNode, const std::function<bool(metacg::CgNode*)>& pattern,
                           std::ostringstream& debugString);
};
}  // namespace LoadImbalance

#endif

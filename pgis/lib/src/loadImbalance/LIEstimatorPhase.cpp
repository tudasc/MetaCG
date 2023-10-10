/**
 * File: LIEstimatorPhase.cpp
 * License: Part of the metacg project. Licensed under BSD 3 clause license. See LICENSE.txt file at
 * https://github.com/tudasc/metacg/LICENSE.txt
 */

#include "loadImbalance/LIEstimatorPhase.h"
#include "CgHelper.h"
#include "MetaData/PGISMetaData.h"

#include <loadImbalance/LIMetaData.h>
#include <loadImbalance/metric/EfficiencyMetric.h>
#include <loadImbalance/metric/ImbalancePercentageMetric.h>
#include <loadImbalance/metric/VariationCoeffMetric.h>
#include <queue>
#include <spdlog/spdlog.h>
#include <sstream>
#include <unordered_map>

using namespace metacg;
using namespace LoadImbalance;

LIEstimatorPhase::LIEstimatorPhase(std::unique_ptr<LIConfig> &&config, metacg::Callgraph *cg)
    : EstimatorPhase("LIEstimatorPhase", cg) {
  this->c = std::move(config);

  if (c->metricType == MetricType::Efficiency) {
    this->metric = new EfficiencyMetric();
  } else if (c->metricType == MetricType::VariationCoeff) {
    this->metric = new VariationCoeffMetric();
  } else if (c->metricType == MetricType::ImbalancePercentage) {
    this->metric = new ImbalancePercentageMetric();
  }
}

LIEstimatorPhase::~LIEstimatorPhase() { delete this->metric; }

void LIEstimatorPhase::modifyGraph(metacg::CgNode *mainMethod) {
  double totalRuntime = mainMethod->get<pira::BaseProfileData>()->getInclusiveRuntimeInSeconds();

  // make sure no node is marked for instrumentation yet
  for (const auto &elem : graph->getNodes()) {
    const auto &n = elem.second.get();
    //    n->setState(CgNodeState::NONE);
    pgis::resetInstrumentation(n);
  }

  instrument(mainMethod);  // keep main method instrumented at all times

  // take notes about imbalanced nodes (for output)
  std::vector<metacg::CgNode *> imbalancedNodeSet;

  for (const auto &elem : graph->getNodes()) {
    const auto &n = elem.second.get();
    // only visit nodes with profiling information which have not yet been marked as irrelevant

    if (n->getOrCreateMD<pira::PiraOneData>()->comesFromCube() &&
        !n->getOrCreateMD<LIMetaData>()->isFlagged(FlagType::Irrelevant)) {
      spdlog::get("console")->debug("LIEstimatorPhase: Processing node " + n->getFunctionName());

      // flag node as visited
      n->get<LIMetaData>()->flag(FlagType::Visited);

      double runtime = n->get<pira::BaseProfileData>()->getInclusiveRuntimeInSeconds();

      std::ostringstream debugString;

      debugString << "Visiting node " << n->getFunctionName() << " ("
                  << n->get<LoadImbalance::LIMetaData>()->getNumberOfInclusiveStatements() << "): ";

      // check whether node is sufficiently important
      if (runtime / totalRuntime >= c->relevanceThreshold) {
        debugString << "important (" << runtime << " / " << totalRuntime << " = " << runtime / totalRuntime << ") (";

        pira::Statements statementThreshold = 0;
        if (c->childRelevanceStrategy == ChildRelevanceStrategy::All) {
          statementThreshold = 0;
        } else if (c->childRelevanceStrategy == ChildRelevanceStrategy::ConstantThreshold) {
          statementThreshold = c->childConstantThreshold;
        } else if (c->childRelevanceStrategy == ChildRelevanceStrategy::RelativeToParent) {
          statementThreshold =
              std::max((pira::Statements)(n->get<LoadImbalance::LIMetaData>()->getNumberOfInclusiveStatements() *
                                          c->childFraction),
                       c->childConstantThreshold);
        } else if (c->childRelevanceStrategy == ChildRelevanceStrategy::RelativeToMain) {
          statementThreshold = std::max(
              (pira::Statements)(mainMethod->get<LoadImbalance::LIMetaData>()->getNumberOfInclusiveStatements() *
                                 c->childFraction),
              c->childConstantThreshold);
        }

        instrumentRelevantChildren(n, statementThreshold, debugString);

        debugString << ") ";

        // check for load imbalance
        this->metric->setNode(n, debugString);
        double m = metric->calc();
        debugString << " -> " << m;
        if (m >= c->imbalanceThreshold) {
          debugString << " => imbalanced";
          n->get<LoadImbalance::LIMetaData>()->setAssessment(m);
          n->get<LoadImbalance::LIMetaData>()->flag(FlagType::Imbalanced);
          imbalancedNodeSet.push_back(n);

          instrument(n);  // make sure imbalanced functions stays instrumented

        } else {
          debugString << " => balanced";
          // mark as irrelevant
          n->get<LIMetaData>()->flag(FlagType::Irrelevant);
        }
      } else {
        debugString << "ignored (" << runtime << " / " << totalRuntime << " = " << runtime / totalRuntime << ")";
        // mark as irrelevant
        n->get<LIMetaData>()->flag(FlagType::Irrelevant);
      }
      spdlog::get("console")->debug(debugString.str());
    }
  }

  // after all nodes have been checked for imbalance and iterative descent has been performed:
  // ContextHandling for imbalanced nodes:
  metacg::analysis::ReachabilityAnalysis ra(graph);
  for (const auto &elem : graph->getNodes()) {
    const auto &n = elem.second.get();
    if (n->getOrCreateMD<LoadImbalance::LIMetaData>()->isFlagged(FlagType::Imbalanced)) {
      contextHandling(n, mainMethod, ra);
    }
  }

  // print summary of detected functions
  std::ostringstream imbalancedNames;
  for (const auto &i : imbalancedNodeSet) {
    imbalancedNames << i->getFunctionName();
    imbalancedNames << " load imbalance assessment: " << i->get<LoadImbalance::LIMetaData>()->getAssessment().value();
    imbalancedNames << " incl. runtime: " << i->get<pira::BaseProfileData>()->getInclusiveRuntimeInSeconds() << " sec.";
    imbalancedNames << "\n";
  }
  spdlog::get("console")->info("Load imbalance summary: " + imbalancedNames.str());
}

void LIEstimatorPhase::instrumentRelevantChildren(metacg::CgNode *node, pira::Statements statementThreshold,
                                                  std::ostringstream &debugString) {
  std::queue<metacg::CgNode *> workQueue;
  CgNodeRawPtrUSet visitedSet;
  for (auto &child : graph->getCallees(node)) {
    workQueue.push(child);
  }

  while (!workQueue.empty()) {
    metacg::CgNode *child = workQueue.front();
    workQueue.pop();

    // no double processing of child (or grandchilds)
    if (visitedSet.find(child) != visitedSet.end()) {
      // already processed
      continue;
    }
    visitedSet.insert(child);

    // process grandchilds (as possible implementations of virtual functions are children of those)
    if (child->get<LoadImbalance::LIMetaData>()->isVirtual()) {
      for (metacg::CgNode *gc : graph->getCallees(child)) {
        workQueue.push(gc);
      }
    }

    if (child->get<LoadImbalance::LIMetaData>()->getNumberOfInclusiveStatements() >= statementThreshold) {
      if (!child->get<LIMetaData>()->isFlagged(FlagType::Irrelevant)) {
        instrument(child);
        debugString << child->getFunctionName() << " ("
                    << child->get<LoadImbalance::LIMetaData>()->getNumberOfInclusiveStatements() << ") ";
      } else {
        debugString << "-" << child->getFunctionName() << "- ("
                    << child->get<LoadImbalance::LIMetaData>()->getNumberOfInclusiveStatements() << ") ";
      }
    } else {
      debugString << "/" << child->getFunctionName() << "\\ ("
                  << child->get<LoadImbalance::LIMetaData>()->getNumberOfInclusiveStatements() << ") ";
    }
  }
}

void LoadImbalance::LIEstimatorPhase::contextHandling(metacg::CgNode *n, metacg::CgNode *mainNode,
                                                      metacg::analysis::ReachabilityAnalysis &ra) {
  if (c->contextStrategy == ContextStrategy::None) {
    return;
  }

  if (c->contextStrategy == ContextStrategy::FindSynchronizationPoints) {
    findSyncPoints(n);
    return;
  }

  CgNodeRawPtrUSet nodesOnPathToMain;

  auto nodesToMain = CgHelper::allNodesToMain(n, mainNode, graph, ra);
  for (auto ntm : nodesToMain) {
    nodesOnPathToMain.insert(ntm);
  }

  CgNodeRawPtrUSet relevantPaths = nodesOnPathToMain;

  if (c->contextStrategy == ContextStrategy::MajorPathsToMain ||
      c->contextStrategy == ContextStrategy::MajorParentSteps) {
    for (metacg::CgNode *x : nodesOnPathToMain) {
      if (!x->get<LIMetaData>()->isFlagged(FlagType::Visited)) {
        relevantPaths.erase(x);
      }
    }
  }

  CgNodeRawPtrUSet toInstrument = relevantPaths;
  if (c->contextStrategy == ContextStrategy::MajorParentSteps) {
    for (metacg::CgNode *x : relevantPaths) {
      if (!reachableInNSteps(x, n, c->contextStepCount)) {
        toInstrument.erase(x);
      }
    }
  }

  // apply instrumentation
  for (metacg::CgNode *x : toInstrument) {
    instrument(x);
  }
}

bool LIEstimatorPhase::reachableInNSteps(metacg::CgNode *start, metacg::CgNode *end, int steps) {
  struct Task {
    CgNode *node;
    int remainingSteps;
  };

  CgNodeRawPtrUSet visitedSet;
  std::queue<Task> taskQueue;
  taskQueue.push({start, steps});

  while (!taskQueue.empty()) {
    Task currentTask = taskQueue.front();
    taskQueue.pop();

    // no double processing
    if (visitedSet.find(currentTask.node) != visitedSet.end()) {
      continue;
    }
    visitedSet.insert(currentTask.node);

    // success
    if (currentTask.node == end) {
      return true;
    }

    // go deeper
    if (currentTask.remainingSteps > 0) {
      for (CgNode *child : graph->getCallees(currentTask.node)) {
        taskQueue.push({child, currentTask.remainingSteps - 1});
      }
    }
  }

  // failure
  return false;
}

void LIEstimatorPhase::instrument(CgNode *node) { pgis::instrumentNode(node); }
// node->setState(CgNodeState::INSTRUMENT_WITNESS); }

void LIEstimatorPhase::findSyncPoints(CgNode *node) {
  std::ostringstream debugString;

  debugString << "LI Detection: Find synchronization points for node " << node->getFunctionName() << "(";

  // process all parents which are balanced + visisted
  for (CgNode *parent : graph->getCallers(node)) {
    if (!parent->get<LoadImbalance::LIMetaData>()->isFlagged(FlagType::Imbalanced) &&
        parent->get<LoadImbalance::LIMetaData>()->isFlagged(FlagType::Visited)) {
      // instrument all descendant synchronization routines
      instrumentByPattern(
          parent, [](CgNode *nodeInQuestion) { return nodeInQuestion->getFunctionName().rfind("MPI_", 0) == 0; },
          debugString);
    }
  }

  spdlog::get("console")->debug(debugString.str());
}

void LIEstimatorPhase::instrumentByPattern(CgNode *startNode, std::function<bool(CgNode *)> pattern,
                                           std::ostringstream &debugString) {
  std::queue<CgNode *> workQueue;
  CgNodeRawPtrUSet alreadyVisited;

  workQueue.push(startNode);

  while (!workQueue.empty()) {
    CgNode *node = workQueue.front();
    workQueue.pop();

    // do not process a node twice
    if (alreadyVisited.find(node) == alreadyVisited.end()) {
      alreadyVisited.insert(node);
      for (CgNode *child : graph->getCallees(node)) {
        workQueue.push(child);
        if (pattern(child)) {
          // mark for call-site instrumentation
          // Fixme this is probably important, and should be moved to metadata
          // child->instrumentFromParent(node);
          debugString << " " << node->getFunctionName() << "->" << child->getFunctionName();
        }
      }
    }
  }
}

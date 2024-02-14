/**
 * File: EstimatorPhase.h
 * License: Part of the metacg project. Licensed under BSD 3 clause license. See LICENSE.txt file at
 * https://github.com/tudasc/metacg/LICENSE.txt
 */

#ifndef ESTIMATORPHASE_H_
#define ESTIMATORPHASE_H_

#include "Callgraph.h"
#include "CgHelper.h"
#include "CgNode.h"

#include <map>
#include <queue>
#include <set>
#include <string>

/**
 * Helper struct that creates a temporary IC from the CG
 *
 * Used by the EstimatorPhase / MCGProcessor to generate and output the InstrumentationConfiguration after a specific
 * EstimatorPhase.
 */
struct InstrumentationConfiguration {
  std::string phaseName;
  std::unordered_set<std::string> instrumentedNames;
  std::unordered_set<metacg::CgNode *> instrumentedNodes;
  std::unordered_map<std::string, metacg::CgNode *> instrumentedPaths;
  std::unordered_map<metacg::CgNode *, metacg::CgNode *> instrumentedEdges;
};

/**
 * Base class for estimator analyses in PGIS.
 *
 * An estimator phase evaluates _some_ node property to decide if that particular node
 * shall be added to the instrumentation configuration (IC).
 * The pure virtual method modifyGraph needs to be implemented.
 */
class EstimatorPhase {
 public:
  explicit EstimatorPhase(std::string name, metacg::Callgraph *callgraph);
  virtual ~EstimatorPhase() = default;

  virtual void doPrerequisites() {}
  virtual void modifyGraph(metacg::CgNode *mainMethod) = 0;

  void generateIC();

  void injectConfig(Config *config) { this->config = config; }

  InstrumentationConfiguration getIC();
  virtual void printReport();

  void setNoReport() { noReportRequired = true; }

  std::string getName() { return name; }

 protected:
  metacg::Callgraph *graph;

  InstrumentationConfiguration IC;
  std::string name;

  Config *config;
  bool noReportRequired;
};

class NopEstimatorPhase : public EstimatorPhase {
 public:
  NopEstimatorPhase(metacg::Callgraph* cg) : EstimatorPhase("NopEstimator", cg), didRun(false) {}
  void modifyGraph(metacg::CgNode *mainMethod) final { didRun = true; }
  bool didRun;
};

/**
 * Instrument according to white list
 */
class WLInstrEstimatorPhase : public EstimatorPhase {
 public:
  explicit WLInstrEstimatorPhase(const std::string &wlFilePath);
  ~WLInstrEstimatorPhase() override = default;

  void modifyGraph(metacg::CgNode *mainMethod) override;

 private:
  std::set<size_t> whiteList;
};

#endif

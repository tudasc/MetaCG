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

struct CgReport {
  CgReport()
      : instrumentedMethods(0),
        overallMethods(0),
        unwConjunctions(0),
        overallConjunctions(0),
        instrumentedCalls(0),
        unwindSamples(0),
        samplesTaken(0),
        instrOvSeconds(.0),
        unwindOvSeconds(.0),
        samplingOvSeconds(.0),
        overallSeconds(.0),
        instrOvPercent(.0),
        unwindOvPercent(.0),
        samplingOvPercent(.0),
        overallPercent(.0),
        phaseName(std::string()),
        metaPhase(false),
        instrumentedNames(),
        instrumentedNodes(),
        instrumentedEdges() {}

  unsigned int instrumentedMethods;
  unsigned int overallMethods;

  unsigned int unwConjunctions;
  unsigned int overallConjunctions;

  unsigned long long instrumentedCalls;
  unsigned long long unwindSamples;
  unsigned long long samplesTaken;

  double instrOvSeconds;
  double unwindOvSeconds;
  double samplingOvSeconds;
  double overallSeconds;

  double instrOvPercent;
  double unwindOvPercent;
  double samplingOvPercent;
  double overallPercent;

  std::string phaseName;
  bool metaPhase;

  std::unordered_set<std::string> instrumentedNames;
  std::unordered_set<metacg::CgNode *> instrumentedNodes;
  std::unordered_map<std::string, metacg::CgNode *> instrumentedPaths;
  std::unordered_map<metacg::CgNode *, metacg::CgNode *> instrumentedEdges;
  //  std::priority_queue<metacg::CgNode*, std::vector<metacg::CgNode*>, CalledMoreOften> instrumentedNodes;

  std::map<std::string, int> unwoundNames;
};

class EstimatorPhase {
 public:
  explicit EstimatorPhase(std::string name, metacg::Callgraph *callgraph, bool isMetaPhase = false);
  virtual ~EstimatorPhase() = default;

  virtual void doPrerequisites() {}
  virtual void modifyGraph(metacg::CgNode *mainMethod) = 0;

  void generateReport();

  [[deprecated]] void setGraph(metacg::Callgraph *graph);
  void injectConfig(Config *config) { this->config = config; }

  struct CgReport getReport();
  virtual void printReport();

  void setNoReport() { noReportRequired = true; }

  std::string getName() { return name; }

 protected:
  metacg::Callgraph *graph;

  CgReport report;
  std::string name;

  Config *config;
  bool noReportRequired;

  /* print some additional information of the phase */
  virtual void printAdditionalReport();
};

class NopEstimatorPhase : public EstimatorPhase {
 public:
  NopEstimatorPhase(metacg::Callgraph* cg) : EstimatorPhase("NopEstimator", cg), didRun(false) {}
  void modifyGraph(metacg::CgNode *mainMethod) final { didRun = true; }
  bool didRun;
};

/**
 * Remove nodes from the graph that are not connected to the main() method.
 *
 * TODO: I guess this can be removed by now...?
 */
class RemoveUnrelatedNodesEstimatorPhase : public EstimatorPhase {
 public:
  explicit RemoveUnrelatedNodesEstimatorPhase(metacg::Callgraph *cg, bool onlyRemoveUnrelatedNodes = true,
                                              bool aggressiveReduction = false);
  ~RemoveUnrelatedNodesEstimatorPhase() override;

  void modifyGraph(metacg::CgNode *mainMethod) override;

 private:
  void printAdditionalReport() override;
  void checkLeafNodeForRemoval(metacg::CgNode *node);

  int numUnconnectedRemoved;
  int numLeafsRemoved;
  int numChainsRemoved;
  int numAdvancedOptimizations;

  bool aggressiveReduction;
  bool onlyRemoveUnrelatedNodes;

  CgNodeRawPtrUSet nodesToRemove;
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

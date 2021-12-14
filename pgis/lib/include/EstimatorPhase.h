/**
 * File: EstimatorPhase.h
 * License: Part of the MetaCG project. Licensed under BSD 3 clause license. See LICENSE.txt file at
 * https://github.com/tudasc/metacg/LICENSE.txt
 */

#ifndef ESTIMATORPHASE_H_
#define ESTIMATORPHASE_H_

#include "Callgraph.h"
#include "CgHelper.h"
#include "CgNode.h"

#include <map>
#include <queue>
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
  std::unordered_map<std::string, CgNodePtr> instrumentedPaths;
  std::priority_queue<CgNodePtr, std::vector<CgNodePtr>, CalledMoreOften> instrumentedNodes;
  std::unordered_map<CgNodePtr, CgNodePtr> instrumentedEdges;

  std::map<std::string, int> unwoundNames;
};

class EstimatorPhase {
 public:
  EstimatorPhase(std::string name, bool isMetaPhase = false);
  virtual ~EstimatorPhase() {}

  virtual void modifyGraph(CgNodePtr mainMethod) = 0;

  void generateReport();

  void setGraph(Callgraph *graph);
  void injectConfig(Config *config) { this->config = config; }

  struct CgReport getReport();
  virtual void printReport();

  void setNoReport() { noReportRequired = true; }

  std::string getName() { return name; }

 protected:
  Callgraph *graph;

  CgReport report;
  std::string name;

  Config *config;
  bool noReportRequired;

  /* print some additional information of the phase */
  virtual void printAdditionalReport();
};

class NopEstimatorPhase : public EstimatorPhase {
 public:
  NopEstimatorPhase() : EstimatorPhase("NopEstimator"), didRun(false) {}
  virtual void modifyGraph(CgNodePtr mainMethod) final { didRun = true; }
  bool didRun;
};

/**
 * Remove nodes from the graph that are not connected to the main() method.
 */
class RemoveUnrelatedNodesEstimatorPhase : public EstimatorPhase {
 public:
  RemoveUnrelatedNodesEstimatorPhase(bool onlyRemoveUnrelatedNodes = true, bool aggressiveReduction = false);
  ~RemoveUnrelatedNodesEstimatorPhase();

  void modifyGraph(CgNodePtr mainMethod);

 private:
  void printAdditionalReport();
  void checkLeafNodeForRemoval(CgNodePtr node);

  int numUnconnectedRemoved;
  int numLeafsRemoved;
  int numChainsRemoved;
  int numAdvancedOptimizations;

  bool aggressiveReduction;
  bool onlyRemoveUnrelatedNodes;

  CgNodePtrSet nodesToRemove;
};

/**
 * Read out some statistics about the current call graph
 */
class GraphStatsEstimatorPhase : public EstimatorPhase {
 public:
  GraphStatsEstimatorPhase();
  ~GraphStatsEstimatorPhase();

  void modifyGraph(CgNodePtr mainMethod);

 private:
  void printAdditionalReport();
  bool hasDependencyFor(CgNodePtr conjunction) {
    for (auto dependency : dependencies) {
      if (dependency.dependentConjunctions.find(conjunction) != dependency.dependentConjunctions.end()) {
        return true;
      }
    }
    return false;
  }

 private:
  struct ConjunctionDependency {
    CgNodePtrSet dependentConjunctions;
    CgNodePtrSet markerPositions;

    ConjunctionDependency(CgNodePtrSet dependentConjunctions, CgNodePtrSet markerPositions) {
      this->dependentConjunctions = dependentConjunctions;
      this->markerPositions = markerPositions;
    }
  };

 private:
  int numCyclesDetected;

  int numberOfConjunctions;
  std::vector<ConjunctionDependency> dependencies;
  std::set<CgNodePtr> allValidMarkerPositions;
};

/**
 * Instrument according to white list
 */
class WLInstrEstimatorPhase : public EstimatorPhase {
 public:
  WLInstrEstimatorPhase(std::string wlFilePath);
  ~WLInstrEstimatorPhase() {}

  void modifyGraph(CgNodePtr mainMethod);

 private:
  std::set<std::string> whiteList;
};

/** reset all instrumented and unwound nodes on the call graph */
class ResetEstimatorPhase : public EstimatorPhase {
 public:
  ResetEstimatorPhase();
  ~ResetEstimatorPhase();

  void printReport() override;
  void modifyGraph(CgNodePtr mainMethod);
};

#endif

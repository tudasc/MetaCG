/**
 * File: PiraMCGProcessor.h
 * License: Part of the metacg project. Licensed under BSD 3 clause license. See LICENSE.txt file at
 * https://github.com/tudasc/metacg/LICENSE.txt
 */

#ifndef PGIS_CALLGRAPHMANAGER_H
#define PGIS_CALLGRAPHMANAGER_H

// clang-format off
// From graph library
#include "Callgraph.h"
#include "CgNode.h"

// From PGIS library
#include "EstimatorPhase.h"
#include "ExtrapConnection.h"
#include "libIPCG/MetaDataHandler.h"

// System library
#include <map>
#include <numeric>  // for std::accumulate
#include <queue>
#include <string>
// clang-format on

namespace metacg::pgis {
/**
 * PIRA's analyzer implemented as a MetaCG Processor.
 * A processor receives the Callgraph and processes it an a suitable way.
 */
class PiraMCGProcessor {
 public:
  /**
   * Get the current call graph instance
   * @return
   */
  static PiraMCGProcessor &get() {
    static PiraMCGProcessor instance;
    return instance;
  }

  void setConfig(Config *cfg) { configPtr = cfg; }

  void setExtrapConfig(extrapconnection::ExtrapConfig epCfg) {
    epModelProvider = extrapconnection::ExtrapModelProvider(epCfg);
  }

  extrapconnection::ExtrapModelProvider &getModelProvider() { return epModelProvider; }

  void printMainRuntime() {
    const auto mainNode = graph.getMain();
    const auto inclTime = mainNode->get<pira::BaseProfileData>()->getInclusiveRuntimeInSeconds();
    spdlog::get("console")->info("Runtime of main is: {}", inclTime);
  }

 private:
  PiraMCGProcessor() : graph(getEmptyGraph()), configPtr(nullptr), epModelProvider({}){};
  explicit PiraMCGProcessor(Config *config, extrapconnection::ExtrapConfig epCfg = {});

  PiraMCGProcessor(const PiraMCGProcessor &other) = default;
  PiraMCGProcessor(PiraMCGProcessor &&other) = default;

  PiraMCGProcessor &operator=(const PiraMCGProcessor &other) = delete;
  PiraMCGProcessor &operator=(PiraMCGProcessor &&other) = default;

 public:
  ~PiraMCGProcessor() = default;

  void registerEstimatorPhase(EstimatorPhase *phase, bool noReport = false);
  void removeAllEstimatorPhases() {
    while (!phases.empty()) {
      phases.pop();
    }
  }
  bool hasPassesRegistered() { return phases.size(); }

  /**
   * @brief applyRegisteredPhases Runs the registered EstimatorPhases in order on the Callgraph.
   */
  void applyRegisteredPhases();

  // Delegates to the underlying graph
  metacg::Callgraph::ContainerT::iterator begin() const { return graph.begin(); }
  metacg::Callgraph::ContainerT::iterator end() const { return graph.end(); }
  metacg::Callgraph::ContainerT::const_iterator cbegin() const { return graph.cbegin(); }
  metacg::Callgraph::ContainerT::const_iterator cend() const { return graph.cend(); }
  size_t size() { return graph.size(); }

  int getNumProcs();
  void printDOT(std::string prefix);
  bool readWhitelist(std::vector<std::string> &whiteNodes);
  bool isNodeListed(std::vector<std::string> whiteNodes, std::string node);
  Callgraph &getCallgraph(PiraMCGProcessor *);
  void setNoOutput() { noOutputRequired = true; }
  void setOutputDotBetweenPhases(bool val = true) {outputDotBetweenPhases = val;}
  bool getOutputDotBetweenPhases(){
    return outputDotBetweenPhases;
  }

  //  void setScorepOutputFormat(bool val = true) { scorepOutput_ = val; }

  void attachExtrapModels();

  void setCG(Callgraph& newGraph) {graph = newGraph; }

 private:
  // this set represents the call graph during the actual computation
  Callgraph &graph;
  Config *configPtr;
  bool noOutputRequired{false};
  bool outputDotBetweenPhases {false};
  //  bool scorepOutput_ = false;

  // Extrap interaction
  extrapconnection::ExtrapModelProvider epModelProvider;

  // estimator phases run in a defined order
  std::queue<EstimatorPhase *> phases;
  std::vector<std::shared_ptr<EstimatorPhase>> donePhases;

  void finalizeGraph(bool finalizeGraph = false);

  void dumpInstrumentedNames(CgReport report);
  void dumpUnwoundNames(CgReport report);
};
}  // namespace metacg::pgis

#endif

/**
 * File: CallgraphManager.h
 * License: Part of the MetaCG project. Licensed under BSD 3 clause license. See LICENSE.txt file at https://github.com/tudasc/metacg/LICENSE.txt
 */

#ifndef CALLGRAPHMANAGER_H
#define CALLGRAPHMANAGER_H

#include "Callgraph.h"
#include "CgNode.h"
#include "EstimatorPhase.h"
#include "ExtrapConnection.h"

#include "libIPCG/MetaDataHandler.h"

#include <map>
#include <numeric>  // for std::accumulate
#include <queue>
#include <string>

#define PRINT_DOT_AFTER_EVERY_PHASE true
#define DUMP_INSTRUMENTED_NAMES true
#define DUMP_UNWOUND_NAMES true

class CallgraphManager {
 public:
  static CallgraphManager &get() {
    static CallgraphManager instance;
    return instance;
  }

  void clear() {
    graph.clear();
    removeAllEstimatorPhases();
    donePhases.clear();
    metaHandlers.clear();
  }

  std::vector<MetaCG::io::retriever::MetaDataHandler *> getMetaHandlers() {
    std::vector<MetaCG::io::retriever::MetaDataHandler *> handler;
    for (const auto &mh : metaHandlers) {
      handler.push_back(mh.get());
    }
    return handler;
  }

  // XXX Move to private once things work
  std::vector<std::unique_ptr<MetaCG::io::retriever::MetaDataHandler>> metaHandlers;
  template <typename T, typename... Args>
  void addMetaHandler(Args... args) {
    metaHandlers.emplace_back(std::make_unique<T>(args...));
    metaHandlers.back()->registerCGManager(this);
  }

  void setConfig(Config *cfg) { config = cfg; }

  void setExtrapConfig(extrapconnection::ExtrapConfig epCfg) {
    epModelProvider = extrapconnection::ExtrapModelProvider(epCfg);
  }

  extrapconnection::ExtrapModelProvider &getModelProvider() { return epModelProvider; }

 private:
  CallgraphManager() : config(nullptr), epModelProvider({}){};
  CallgraphManager(Config *config, extrapconnection::ExtrapConfig epCfg = {});

  CallgraphManager(const CallgraphManager &other) = default;
  CallgraphManager(CallgraphManager &&other) = default;

  CallgraphManager &operator=(const CallgraphManager &other) = delete;
  CallgraphManager &operator=(CallgraphManager &&other) = default;

 public:
  ~CallgraphManager() {}

  // legacy version without inclusive time
  void putEdge(const std::string &parentName, std::string parentFilename, int parentLine, std::string childName,
               unsigned long long numberOfCalls, double timeInSeconds, int threadId, int procId);

  void putEdge(const std::string &parentName, std::string parentFilename, int parentLine, std::string childName,
               unsigned long long numberOfCalls, double timeInSeconds, double inclusiveTimeInSeconds, int threadId,
               int procId);

  void putNumberOfStatements(std::string name, int numberOfStatements, bool hasBody = true);
  void putNumberOfSamples(std::string name, unsigned long long numberOfSamples);
  CgNodePtr findOrCreateNode(std::string name, double timeInSeconds = 0.0);
  void setNodeComesFromCube(std::string name);

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
  Callgraph::ContainerT::iterator begin() const { return graph.begin(); }
  Callgraph::ContainerT::iterator end() const { return graph.end(); }
  Callgraph::ContainerT::const_iterator cbegin() const { return graph.cbegin(); }
  Callgraph::ContainerT::const_iterator cend() const { return graph.cend(); }
  size_t size() { return graph.size(); }

  int getNumProcs();
  void printDOT(std::string prefix);
  bool readWhitelist(std::vector<std::string> &whiteNodes);
  bool isNodeListed(std::vector<std::string> whiteNodes, std::string node);
  Callgraph &getCallgraph(CallgraphManager *);
  void setNoOutput() { noOutputRequired = true; }

  void setScorepOutputFormat(bool val = true) { scorepOutput_ = val; }

  void attachExtrapModels();

  void putEdge(std::string parentName, std::string childName);

 private:
  // this set represents the call graph during the actual computation
  Callgraph graph;
  Config *config;
  bool noOutputRequired = false;
  bool scorepOutput_ = false;

  // Extrap interaction
  extrapconnection::ExtrapModelProvider epModelProvider;

  // estimator phases run in a defined order
  std::queue<EstimatorPhase *> phases;
  std::vector<std::shared_ptr<EstimatorPhase>> donePhases;

  void finalizeGraph(bool finalizeGraph = false);

  void dumpInstrumentedNames(CgReport report);
  void dumpUnwoundNames(CgReport report);
};

#endif

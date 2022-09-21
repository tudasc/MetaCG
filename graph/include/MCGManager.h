/**
 * File: MCGManager.h
 * License: Part of the MetaCG project. Licensed under BSD 3 clause license. See LICENSE.txt file at
 * https://github.com/tudasc/metacg/LICENSE.txt
 */
#ifndef METACG_GRAPH_MCGMANAGER_H
#define METACG_GRAPH_MCGMANAGER_H

#include "Callgraph.h"
#include "MCGReader.h"
#include "LoggerUtil.h"

namespace metacg {
// This is part of the core graph library
namespace graph {

/**
 * High-level interface to create and import/export the CG.
 * Also used by the MetaDataHandler for inserting their respective data and additional nodes.
 */
class MCGManager {
 public:
  static MCGManager &get() {
    static MCGManager instance;
    return instance;
  }

  /**
   * Resets the the whole manager
   */
  void resetManager() {
    metaHandlers.clear();
    managedGraphs.clear();
    activeGraph = nullptr;
  }

  /**
   * Resets the current active graph to be empty
   */
  bool resetActiveGraph() {
    if (activeGraph) {
      activeGraph->clear();
      // just because a graph exists, doesn't mean it has attached metadata
      //  I think this is the best way to do it
      //  until map::contains comes with c++20
      //  I expect most callgraphs to have attached metadata
      //  so the exception should be rare
      try {
        metaHandlers.at(activeGraph).clear();
      } catch (const std::out_of_range &ex) {
      }
      return true;
    } else {
      metacg::MCGLogger::instance().getErrConsole()->error("Graph manager could not reset active Graph, no active graph exists.");
      assert(false && "Graph manager could not reset active Graph, no active graph exists");
      return false;
    }
  }

  /**
   * Registers the MetaDataHandler and takes ownership of the object.
   * @tparam T
   * @tparam Args
   * @param args
   */
  template <typename T, typename... Args>
  bool addMetaHandler(Args... args) {
    if (activeGraph != nullptr) {
      metaHandlers[activeGraph].emplace_back(std::make_unique<T>(args...));
      metaHandlers[activeGraph].back()->registerMCGManager(this);
      return true;
    } else {
      metacg::MCGLogger::instance().getErrConsole()->error("Graph manager could not add metadata handler, no active graph exists");
      assert(false && "Graph manager could not add metadata handler, no active graph exists");
      return false;
    }
  }

  /**
   * Returns list of non-owned pointers to MetaDataHandler.
   * @return
   */
  std::vector<metacg::io::retriever::MetaDataHandler *> getMetaHandlers() const {
    std::vector<metacg::io::retriever::MetaDataHandler *> handler;
    // the tests expect an empty vector of metadata handlers to be returned if the active graph is either not metadata
    // annotated or doesn't exist
    if (metaHandlers.count(activeGraph) != 0) {
      for (const auto &mh : metaHandlers.at(activeGraph)) {
        handler.push_back(mh.get());
      }
    }
    return handler;
  }

  /**
   * Inserts an edge from parentName to childName
   * @param parentName function name of calling function
   * @param childName function name of called function
   */
  // Note: this is kept for compatibility reasons
  //  remove this once transition is complete
  bool addEdge(const std::string &parentName, const std::string &childName) {
    if (activeGraph != nullptr) {
      activeGraph->addEdge(parentName, childName);
      return true;
    } else {
      metacg::MCGLogger::instance().getErrConsole()->error("Graph manager could not create edge between %s and %s, no active graph exists", parentName, childName );
      assert(false && "Graph manager could not create edge between given nodes, no active graph exists");
      return false;
    }
  }

  /**
   * Inserts an edge from parentNode to childNode
   * @param parentNode function node of calling function
   * @param childNode function node of called function
   */
  // Note: this is kept for compatibility reasons
  //  remove this once transition is complete
  bool addEdge(CgNodePtr parentNode, CgNodePtr childNode) {
    if (activeGraph != nullptr) {
      activeGraph->addEdge(parentNode, childNode);
      return true;
    } else {
      metacg::MCGLogger::instance().getErrConsole()->error("Graph manager could not create edge, no active graph exists\n");
      assert(false && "Graph manager could not create edge, no active graph exists");
      return false;
    }
  }
  /**
   * Returns the node for given identifier, if no node exists yet, it creates a new one.
   * @param name name to identify the node by
   * @returns a shared pointer to the requested node
   */

  // Note: this is kept for compatibility reasons
  //  remove this once transition is complete
  CgNodePtr findOrCreateNode(const std::string &name) { return activeGraph->getOrInsertNode(name); }

  // Delegates to the underlying graph
  metacg::Callgraph::ContainerT::iterator begin() { return activeGraph->begin(); }
  metacg::Callgraph::ContainerT::iterator end() { return activeGraph->end(); }
  metacg::Callgraph::ContainerT::const_iterator cbegin() const { return activeGraph->cbegin(); }
  metacg::Callgraph::ContainerT::const_iterator cend() const { return activeGraph->cend(); }
  size_t size() { return activeGraph->size(); }

  // Delegates iterators for all managed graphs
  std::unordered_map<std::string, std::unique_ptr<metacg::Callgraph>>::iterator graphs_begin() {
    return managedGraphs.begin();
  }
  std::unordered_map<std::string, std::unique_ptr<metacg::Callgraph>>::iterator graphs_end() {
    return managedGraphs.end();
  }
  std::unordered_map<std::string, std::unique_ptr<metacg::Callgraph>>::const_iterator graphs_cbegin() const {
    return managedGraphs.cbegin();
  }
  std::unordered_map<std::string, std::unique_ptr<metacg::Callgraph>>::const_iterator graphs_cend() const {
    return managedGraphs.cend();
  }

  size_t graphs_size() const { return managedGraphs.size(); }

  [[nodiscard]] metacg::Callgraph *getCallgraph() { return activeGraph; }
  [[nodiscard]] metacg::Callgraph *getOrCreateCallgraph(const std::string &name) { return managedGraphs[name].get(); }

  /**
   * Sets a callgraph to be the currently active one
   * @param callgraph the name the callgraph is identified by
   * @returns true if the callgraph could be set active,\n false if it is not part of the managed callgraphs
   *
   **/
  // Todo: write a test for this
  bool setActive(const std::string &callgraph) {
    // I think this is the best way to do it
    // until map::contains comes with c++20
    // another implementation would be to
    // generate a new graph if no matching graph was found
    // I expect most users to only set a graph active
    // once it has been added, so the exception should be rare
    try {
      activeGraph = managedGraphs.at(callgraph).get();
    } catch (const std::out_of_range &ex) {
      metacg::MCGLogger::instance().getErrConsole()->error("Could not set graph %s to active, graph does not exist",callgraph);
      assert(false && "Could not set graph to active, graph does not exist");
      return false;
    }
    return true;
  }

  /**
   * Checks if a callgraph is the currently active one
   * @param callgraph the name the callgraph is identified by
   * @returns true if the callgraph is the active one \n false if the callgraph is inactive or doesn't exist
   **/
  // TODO: write a test for this
  bool assertActive(const std::string &callgraph) {
    try {
      return managedGraphs.at(callgraph).get() == activeGraph;
    } catch (const std::out_of_range &ex) {
      metacg::MCGLogger::instance().getErrConsole()->error("Graph: %s is not part of managed graphs",callgraph);
      assert(false && "Graph is not part of managed graphs");
      return false;
    }
  }

  /**
   * Transfers ownership of a graph to the graph manager
   * @param name the name to identify the graph with
   * @param callgraph the unique pointer to the callgraph
   * @param setActive optional flag to set the newly added graph as the active one
   **/
  // Todo: write a test for this
  bool addToManagedGraphs(std::string name, std::unique_ptr<metacg::Callgraph> callgraph, bool setActive = true) {
    assert(callgraph.get() != nullptr && "Could not add to managed graphs, given graph was null");
    managedGraphs[name] = std::move(callgraph);
    if (setActive) {
      activeGraph = managedGraphs[name].get();
    }
    return true;
  }

  ~MCGManager() = default;

 private:
  MCGManager() = default;
  MCGManager(const MCGManager &other) = default;
  MCGManager(MCGManager &&other) = default;

  MCGManager &operator=(const MCGManager &other) = delete;
  MCGManager &operator=(MCGManager &&other) = delete;

  std::unordered_map<std::string, std::unique_ptr<metacg::Callgraph>> managedGraphs;
  metacg::Callgraph *activeGraph = nullptr;
  std::unordered_map<metacg::Callgraph *, std::vector<std::unique_ptr<metacg::io::retriever::MetaDataHandler>>>
      metaHandlers{};
};
}  // namespace graph
}  // namespace metacg

#endif  // METACG_MCGMANAGER_H

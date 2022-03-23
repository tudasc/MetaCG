/**
 * File: MCGManager.h
 * License: Part of the MetaCG project. Licensed under BSD 3 clause license. See LICENSE.txt file at
 * https://github.com/tudasc/metacg/LICENSE.txt
 */
#ifndef METACG_GRAPH_MCGMANAGER_H
#define METACG_GRAPH_MCGMANAGER_H

#include "Callgraph.h"
#include "MCGReader.h"

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

  void reset() {
    graph.clear();
    metaHandlers.clear();
  }

  /**
   * Registers the MetaDataHandler and takes ownership of the object.
   * @tparam T
   * @tparam Args
   * @param args
   */
  template <typename T, typename... Args>
  void addMetaHandler(Args... args) {
    metaHandlers.emplace_back(std::make_unique<T>(args...));
    metaHandlers.back()->registerMCGManager(this);
  }

  /**
   * Returns list of non-owned pointers to MetaDataHandler.
   * @return
   */
  std::vector<metacg::io::retriever::MetaDataHandler *> getMetaHandlers() {
    std::vector<metacg::io::retriever::MetaDataHandler *> handler;
    for (const auto &mh : metaHandlers) {
      handler.push_back(mh.get());
    }
    return handler;
  }

  /**
   * Inserts an edge from parentName to childName
   * @param parentName function name of calling function
   * @param childName function name of called function
   */
  //Note: this is kept for compatibility reasons
  // remove this once transition is complete
  void addEdge(const std::string &parentName, const std::string &childName) {
    graph.addEdge(parentName, childName);
  };

  /**
   * Inserts an edge from parentNode to childNode
   * @param parentNode function node of calling function
   * @param childNode function node of called function
   */
  //Note: this is kept for compatibility reasons
  // remove this once transition is complete
  void addEdge(CgNodePtr parentNode, CgNodePtr childNode) {
    graph.addEdge(parentNode, childNode);
  }

  /**
   * Returns the node for @param name
   * If no node exists yet, it creates a new one.
   */
   //Note: this is kept for compatibility reasons
   // remove this once transition is complete
  CgNodePtr findOrCreateNode(const std::string &name) {
    return graph.getOrInsertNode(name);
  }

  // Delegates to the underlying graph
  metacg::Callgraph::ContainerT::iterator begin() const { return graph.begin(); }
  metacg::Callgraph::ContainerT::iterator end() const { return graph.end(); }
  metacg::Callgraph::ContainerT::const_iterator cbegin() const { return graph.cbegin(); }
  metacg::Callgraph::ContainerT::const_iterator cend() const { return graph.cend(); }
  size_t size() { return graph.size(); }
  [[nodiscard]] metacg::Callgraph &getCallgraph() { return graph; }

  ~MCGManager() = default;

 private:
  MCGManager() = default;
  MCGManager(const MCGManager &other) = default;
  MCGManager(MCGManager &&other) = default;

  MCGManager &operator=(const MCGManager &other) = delete;
  MCGManager &operator=(MCGManager &&other) = delete;

  metacg::Callgraph graph;
  std::vector<std::unique_ptr<metacg::io::retriever::MetaDataHandler>> metaHandlers{};
};
}  // namespace graph
}  // namespace metacg

#endif  // METACG_MCGMANAGER_H

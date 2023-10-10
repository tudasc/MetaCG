/**
 * File: MCGManager.h
 * License: Part of the MetaCG project. Licensed under BSD 3 clause license. See LICENSE.txt file at
 * https://github.com/tudasc/metacg/LICENSE.txt
 */
#ifndef METACG_GRAPH_MCGMANAGER_H
#define METACG_GRAPH_MCGMANAGER_H

#include "Callgraph.h"
#include <vector>

namespace metacg {
// This is part of the core graph library
namespace graph {
/**
 * High-level interface to create and import/export the CG.
 * Also used by the MetaDataHandler for inserting their respective data and additional nodes.
 */
class MCGManager {
 public:
  static MCGManager &get();

  /**
   * Resets the the whole manager
   */
  void resetManager();

  /**
   * Resets the current active graph to be empty
   */
  bool resetActiveGraph();

  size_t size() const;

  size_t graphs_size() const;

  [[nodiscard]] Callgraph *getCallgraph(const std::string &name = "", bool setActive = false);

  [[nodiscard]] Callgraph *getOrCreateCallgraph(const std::string &name, bool setActive = false);

  /**
   * Sets a callgraph to be the currently active one
   * @param callgraph the name the callgraph is identified by
   * @returns true if the callgraph could be set active,\n false if it is not part of the managed callgraphs
   *
   **/
  // Todo: write a test for this
  bool setActive(const std::string &callgraph);

  /**
   * Checks if a callgraph is the currently active one
   * @param callgraph the name the callgraph is identified by
   * @returns true if the callgraph is the active one \n false if the callgraph is inactive or doesn't exist
   **/
  // TODO: write a test for this
  bool assertActive(const std::string &callgraph);

  /**
   * Transfers ownership of a graph to the graph manager
   * @param name the name to identify the graph with
   * @param callgraph the unique pointer to the callgraph
   * @param setActive optional flag to set the newly added graph as the active one
   **/
  // Todo: write a test for this
  bool addToManagedGraphs(const std::string &name, std::unique_ptr<Callgraph> callgraph, bool setActive = true);

  ~MCGManager();

 private:
  MCGManager() = default;

  MCGManager(const MCGManager &other) = default;

  MCGManager(MCGManager &&other) = default;

  MCGManager &operator=(const MCGManager &other) = delete;

  MCGManager &operator=(MCGManager &&other) = delete;

  std::unordered_map<std::string, std::unique_ptr<Callgraph>> managedGraphs;
  Callgraph *activeGraph = nullptr;
  std::unordered_set<std::string> getAllManagedGraphNames();
};

}  // namespace graph
}  // namespace metacg

#endif  // METACG_MCGMANAGER_H

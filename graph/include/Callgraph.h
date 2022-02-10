/**
 * File: Callgraph.h
 * License: Part of the MetaCG project. Licensed under BSD 3 clause license. See LICENSE.txt file at
 * https://github.com/tudasc/metacg/LICENSE.txt
 */
#ifndef METACG_GRAPH_CALLGRAPH_H
#define METACG_GRAPH_CALLGRAPH_H

#include "CgNode.h"

namespace metacg {

/**
 * Holds the actual call graph.
 */
class Callgraph {
 public:

  Callgraph() : graph(), mainNode(nullptr), lastSearched(nullptr) {}

  // TODO: Can this be a hash set?
  typedef CgNodePtrSet ContainerT;

  /**
   * @brief findMain
   * @return main function CgNodePtr
   */
  CgNodePtr findMain();

  /**
   * @brief findNode
   * @param functionName
   * @return the corresponding CgNodePtr or nullptr
   */
  CgNodePtr findNode(std::string functionName) const;

  /**
   * Inserts a new node and sets it as the 'main' function if its name is main or _Z4main or _ZSt4mainiPPc
   * @param node
   */
  void insert(CgNodePtr node);

  /**
   * Clears the graph to an empty graph with no main node and no lastSearched node.
   */
  void clear() {
    graph.clear();
    lastSearched = nullptr;
    mainNode = nullptr;
  }

  ContainerT::iterator begin() const { return graph.begin(); }
  ContainerT::iterator end() const { return graph.end(); }
  ContainerT::const_iterator cbegin() const;
  ContainerT::const_iterator cend() const;

  bool hasNode(CgNodePtr n) { return graph.find(n) != graph.end(); }

  /**
   * @brief hasNode checks whether a node for #name exists in the graph mapping
   * @param name
   * @return true iff exists, false otherwise
   */
  bool hasNode(std::string name);

  /**
   * @brief getLastSearched - only call after hasNode returned True
   * @return node found by #hasNode - nullptr otherwise
   */
  CgNodePtr getLastSearched() { return lastSearched; }

  /**
   * @brief getNode searches the node in the graph and returns it
   * @param name
   * @return node for function with name - nullptr otherwise
   */
  CgNodePtr getNode(std::string name);

  size_t size() const;
  ContainerT &getGraph();

  bool isEmpty() {
    return graph.size() == 0;
  }

 private:
  // this set represents the call graph during the actual computation
  ContainerT graph;

  // Dedicated node pointer to main function
  CgNodePtr mainNode;

  // Temporary storage for hasNode/getLastSearched combination
  CgNodePtr lastSearched;
};

static Callgraph &getEmptyGraph() {
  static Callgraph graph;
  return graph;
}

}  // namespace metacg
#endif

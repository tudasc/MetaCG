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
   * @brief getMain
   * @return main function CgNodePtr
   */
  CgNodePtr getMain();

  /**
   * Inserts an edge from parentName to childName
   * @param parentName function name of calling function
   * @param childName function name of called function
   */
  void addEdge(const std::string &parentName, const std::string &childName);

  /**
   * Inserts an edge from parentNode to childNode
   * @param parentNode function node of calling function
   * @param childNode function node of called function
   */
  void addEdge(CgNodePtr parentNode, CgNodePtr childNode);

  /**
   * Inserts a new node and sets it as the 'main' function if its name is main or _Z4main or _ZSt4mainiPPc
   * @param node
   */
  void insert(CgNodePtr node);

  /**
   * Returns the node with the given name\n
   * If no node exists yet, it creates a new one.
   * @param name to identify the node by
   * @return CgNodePtr to the identified node
   */
  CgNodePtr getOrInsertNode(const std::string &name);

  /**
   * Clears the graph to an empty graph with no main node and no lastSearched node.
   */
  void clear();

  /**
   * @brief hasNode checks whether a node for #name exists in the graph mapping
   * @param name
   * @return true iff exists, false otherwise
   */
  bool hasNode(std::string name);

  /**
   * @brief hasNode checks whether a node exists in the graph mapping
   * @param node
   * @return true iff exists, false otherwise
   */
  bool hasNode(CgNodePtr n);

  /**
   * @brief getLastSearchedNode - only call after hasNode returned True
   * @return node found by #hasNode - nullptr otherwise
   */
  CgNodePtr getLastSearchedNode();

  /**
   * @brief getNode searches the node in the graph and returns it
   * @param name
   * @return node for function with name - nullptr otherwise
   */
  CgNodePtr getNode(std::string name);

  size_t size() const;

  ContainerT &getGraph();

  bool isEmpty();
  ContainerT::iterator begin() const;
  ContainerT::iterator end() const;

  ContainerT::const_iterator cbegin() const;
  ContainerT::const_iterator cend() const;

 private:
  // this set represents the call graph during the actual computation
  ContainerT graph;

  // Dedicated node pointer to main function
  CgNodePtr mainNode;

  // Temporary storage for hasNode/getLastSearchedNode combination
  CgNodePtr lastSearched;
};

static Callgraph &getEmptyGraph() {
  static Callgraph graph;
  return graph;
}

}  // namespace metacg
#endif

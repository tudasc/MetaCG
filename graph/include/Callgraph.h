/**
 * File: Callgraph.h
 * License: Part of the MetaCG project. Licensed under BSD 3 clause license. See LICENSE.txt file at
 * https://github.com/tudasc/metacg/LICENSE.txt
 */
#ifndef METACG_GRAPH_CALLGRAPH_H
#define METACG_GRAPH_CALLGRAPH_H

#include "CgEdge.h"
#include "CgEdgePtr.h"
#include "CgNode.h"
#include "CgNodePtr.h"
#include <cstddef>

template <>
struct std::hash<std::pair<size_t, size_t>> {
  std::size_t operator()(std::pair<size_t, size_t> const &p) const noexcept {
    std::size_t h1 = std::hash<size_t>{}(p.first);
    std::size_t h2 = std::hash<size_t>{}(p.second);
    return h1 ^ (h2 << 1);  // or use boost::hash_combine
  }
};
namespace metacg {
class Callgraph {
 public:
  // TODO: Can NodeContainer be a set if nameIdMap maps to CgNodePtr?
  typedef std::unordered_map<size_t, CgNodePtr> NodeContainer;
  typedef std::unordered_map<std::string, size_t> NameIdMap;
  typedef std::unordered_map<std::pair<size_t, size_t>, CgEdgePtr> EdgeContainer;
  typedef std::unordered_map<size_t, std::vector<size_t>> CallerList;
  typedef std::unordered_map<size_t, std::vector<size_t>> CalleeList;

  Callgraph() : nodes(), nameIdMap(), edges(), mainNode(nullptr), lastSearched(nullptr) {}

  /**
   * @brief getMain
   * @return main function CgNodePtr
   */
  CgNode *getMain();

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
  void addEdge(const CgNode &parentNode, const CgNode &childNode);

  void addEdge(size_t parentID, size_t childID);

  void addEdge(const CgNode *parent, const CgNode *child);

  /**
   * Inserts a new node and sets it as the 'main' function if its name is main or _Z4main or _ZSt4mainiPPc
   * @param node
   */
  size_t insert(CgNodePtr node);
  size_t insert(const std::string& nodeName);
  /**
   * Returns the node with the given name\n
   * If no node exists yet, it creates a new one.
   * @param name to identify the node by
   * @return CgNodePtr to the identified node
   */
  CgNode *getOrInsertNode(const std::string &name);

  /**
   * Clears the graph to an empty graph with no main node and no lastSearched node.
   */
  void clear();

  /**
   * @brief hasNode checks whether a node for #name exists in the graph mapping
   * @param name
   * @return true iff exists, false otherwise
   */
  bool hasNode(const std::string &name);

  /**
   * @brief hasNode checks whether a node exists in the graph mapping
   * @param node
   * @return true iff exists, false otherwise
   */
  bool hasNode(const CgNode &n);

  bool hasNode(const CgNode *n);
  bool hasNode(const size_t n);

  /**
   * @brief getLastSearchedNode - only call after hasNode returned True
   * @return node found by #hasNode - nullptr otherwise
   */
  CgNode *getLastSearchedNode() const;

  /**
   * @brief getNode searches the node in the graph and returns it
   * @param name
   * @return node for function with name - nullptr otherwise
   */
  CgNode *getNode(const std::string &name) const;
  CgNode *getNode(size_t id) const;

  bool existEdgeFromTo(const CgNode &source, const CgNode &target) const;
  bool existEdgeFromTo(const CgNode *source, const CgNode *&target) const;
  bool existEdgeFromTo(size_t source, size_t target) const;
  bool existEdgeFromTo(const std::string &source, const std::string &target) const;

  CgNodeRawPtrUSet getCallees(const CgNodePtr &node) const;
  CgNodeRawPtrUSet getCallees(size_t node) const;
  CgNodeRawPtrUSet getCallees(const std::string &node) const;
  CgNodeRawPtrUSet getCallees(const CgNode *node) const;

  CgNodeRawPtrUSet getCallers(const CgNodePtr &node) const;
  CgNodeRawPtrUSet getCallers(size_t node) const;
  CgNodeRawPtrUSet getCallers(const std::string &node) const;
  CgNodeRawPtrUSet getCallers(const CgNode *node) const;

  size_t size() const;

  bool isEmpty() const;

  const NodeContainer &getNodes() const;
  const EdgeContainer &getEdges() const;

 private:
  // this set represents the call graph during the actual computation
  NodeContainer nodes;
  NameIdMap nameIdMap;
  EdgeContainer edges;
  CallerList callerList;
  CalleeList calleeList;

  // Dedicated node pointer to main function
  CgNode *mainNode;

  // Temporary storage for hasNode/getLastSearchedNode combination
  CgNode *lastSearched;
};

[[maybe_unused]] static Callgraph& getEmptyGraph() {
  static Callgraph graph;
  return graph;
}
}  // namespace metacg
#endif

/**
 * File: Callgraph.h
 * License: Part of the MetaCG project. Licensed under BSD 3 clause license. See LICENSE.txt file at
 * https://github.com/tudasc/metacg/LICENSE.txt
 */
#ifndef METACG_GRAPH_CALLGRAPH_H
#define METACG_GRAPH_CALLGRAPH_H

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
  typedef std::unordered_map<std::string, MetaData *> NamedMetadata;
  typedef std::unordered_map<std::pair<size_t, size_t>, NamedMetadata> EdgeContainer;
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
  size_t insert(const std::string &nodeName);
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

  CgNodeRawPtrUSet getCallees(const CgNode &node) const;
  CgNodeRawPtrUSet getCallees(const CgNode *node) const;
  CgNodeRawPtrUSet getCallees(size_t node) const;
  CgNodeRawPtrUSet getCallees(const std::string &node) const;

  CgNodeRawPtrUSet getCallers(const CgNode &node) const;
  CgNodeRawPtrUSet getCallers(const CgNode *node) const;
  CgNodeRawPtrUSet getCallers(size_t node) const;
  CgNodeRawPtrUSet getCallers(const std::string &node) const;

  size_t size() const;

  bool isEmpty() const;

  const NodeContainer &getNodes() const;
  const EdgeContainer &getEdges() const;

  void setNodes(NodeContainer external_container);
  void setEdges(EdgeContainer external_container);

  void recomputeCache();

  template <class T>
  void addEdgeMetaData(const CgNode &func1, const CgNode &func2, T *md) {
    addEdgeMetaData({func1.getId(), func2.getId()}, md);
  }
  template <class T>
  void addEdgeMetaData(const CgNode *func1, const CgNode *func2, T *md) {
    addEdgeMetaData({func1->getId(), func2->getId()}, md);
  }
  template <class T>
  void addEdgeMetaData(const std::pair<size_t, size_t> id, T *md) {
    edges.at(id)[T::key] = md;
  }
  template <class T>
  void addEdgeMetaData(const std::string &func1, const std::string &func2, T *md) {
    addEdgeMetaData({nameIdMap.at(func1), nameIdMap.at(func2)}, md);
  }

  MetaData *getEdgeMetaData(const CgNode &func1, const CgNode &func2, const std::string &metadataName) const;
  MetaData *getEdgeMetaData(const CgNode *func1, const CgNode *func2, const std::string &metadataName) const;
  MetaData *getEdgeMetaData(const std::pair<size_t, size_t> id, const std::string &metadataName) const;
  MetaData *getEdgeMetaData(const std::string &func1, const std::string &func2, const std::string &metadataName) const;

  template <class T>
  T *getEdgeMetaData(const CgNode &func1, const CgNode &func2) {
    return getEdgeMetaData<T>({func1.getId(), func2.getId()});
  }
  template <class T>
  T *getEdgeMetaData(const CgNode *func1, const CgNode *func2) {
    return getEdgeMetaData<T>({func1->getId(), func2->getId()});
  }
  template <class T>
  T *getEdgeMetaData(const std::pair<size_t, size_t> id) {
    return static_cast<T>(edges.at(id).at(T::key));
  }
  template <class T>
  T *getEdgeMetaData(const std::string &func1, const std::string &func2) {
    return getEdgeMetaData<T>({nameIdMap.at(func1), nameIdMap.at(func2)});
  }

  const NamedMetadata &getAllEdgeMetaData(const CgNode &func1, const CgNode &func2) const;
  const NamedMetadata &getAllEdgeMetaData(const CgNode *func1, const CgNode *func2) const;
  const NamedMetadata &getAllEdgeMetaData(const std::pair<size_t, size_t> id) const;
  const NamedMetadata &getAllEdgeMetaData(const std::string &func1, const std::string &func2) const;

  bool hasEdgeMetaData(const CgNode &func1, const CgNode &func2, const std::string &metadataName) const;
  bool hasEdgeMetaData(const CgNode *func1, const CgNode *func2, const std::string &metadataName) const;
  bool hasEdgeMetaData(const std::pair<size_t, size_t> id, const std::string &metadataName) const;
  bool hasEdgeMetaData(const std::string &func1, const std::string &func2, const std::string &metadataName) const;

  template <class T>
  bool hasEdgeMetaData(const CgNode &func1, const CgNode &func2) const {
    return hasEdgeMetaData<T>({func1.getId(), func2.getId()});
  }
  template <class T>
  bool hasEdgeMetaData(const CgNode *func1, const CgNode *func2) const {
    return hasEdgeMetaData<T>({func1->getId(), func2->getId()});
  }
  template <class T>
  bool hasEdgeMetaData(const std::pair<size_t, size_t> id) const {
    return edges.at(id).find(T::key) != edges.at(id).end();
  }
  template <class T>
  bool hasEdgeMetaData(const std::string &func1, const std::string &func2) const {
    return hasEdgeMetaData<T>({nameIdMap.at(func1), nameIdMap.at(func2)});
  }

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

[[maybe_unused]] static Callgraph &getEmptyGraph() {
  static Callgraph graph;
  return graph;
}
}  // namespace metacg
#endif

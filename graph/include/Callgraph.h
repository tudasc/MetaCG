/**
 * File: Callgraph.h
 * License: Part of the MetaCG project. Licensed under BSD 3 clause license. See LICENSE.txt file at
 * https://github.com/tudasc/metacg/LICENSE.txt
 */
#ifndef METACG_GRAPH_CALLGRAPH_H
#define METACG_GRAPH_CALLGRAPH_H

#include "CgNode.h"
#include "MergePolicy.h"
#include "Util.h"
#include "metadata/MetadataMixin.h"

template <>
struct std::hash<std::pair<size_t, size_t>> {
  std::size_t operator()(std::pair<size_t, size_t> const& p) const noexcept {
    std::size_t h1 = std::hash<size_t>{}(p.first);
    std::size_t h2 = std::hash<size_t>{}(p.second);
    return h1 ^ (h2 << 1);  // or use boost::hash_combine
  }
};

namespace metacg {

using CgNodeRawPtrUSet = std::unordered_set<metacg::CgNode*>;

class Callgraph : public MetadataMixin {
 public:
  using NodeContainer = std::vector<CgNodePtr>;
  using NodeList = std::vector<NodeId>;

  using NameIdMap = std::unordered_map<std::string, NodeList>;

  using NamedMetadata = std::unordered_map<std::string, std::unique_ptr<MetaData>>;
  using EdgeContainer = std::unordered_map<std::pair<NodeId, NodeId>, NamedMetadata>;
  using CallerList = std::unordered_map<NodeId, NodeList>;
  using CalleeList = std::unordered_map<NodeId, NodeList>;

  // Required for automatic overload resolution
  using MetadataMixin::erase;

  Callgraph() : nodes(), nameIdMap(), edges(), mainNode(nullptr) {}

  ~Callgraph() = default;

  Callgraph(const Callgraph& other) = delete;             // No copy constructor
  Callgraph& operator=(const Callgraph& other) = delete;  // No copy assign constructor

  Callgraph(Callgraph&& other) = default;             // Use default move constructor
  Callgraph& operator=(Callgraph&& other) = default;  // Use default move assign constructor

  /**
   * @brief getMain
   * @return main function CgNodePtr
   */
  CgNode* getMain(bool forceRecompute = false) const;

  /**
   * Inserts an edge from parentNode to childNode.
   * Does nothing if one of the nodes is not contained in this graph.
   * @param parentNode function node of calling function
   * @param childNode function node of called function
   * @return True if the edge was inserted, false otherwise.
   */
  bool addEdge(const CgNode& parentNode, const CgNode& childNode);

  /**
   * Inserts an edge between the nodes with the given IDs.
   * @param parentID
   * @param childID
   * @return True if the the edge was inserted, false otherwise.
   */
  bool addEdge(NodeId parentID, NodeId childID);

  /**
   * Inserts an edge between the two nodes with the given names.
   * Does nothing if there are multiple or no nodes with this name.
   * @param callerName
   * @param calleeName
   * @return True if the edges was inserted, false otherwise.
   */
  bool addEdge(const std::string& callerName, const std::string& calleeName);

  /**
   * Removes the edge between the given nodes.
   * @param parentID
   * @param childID
   * @return True if there was an edge to remove, false otherwise.
   */
  bool removeEdge(NodeId parentID, NodeId childID);

  /**
   * Removes the edge from parentNode to childNode.
   * @param parentNode function node of calling function
   * @param childNode function node of called function
   * @return True if there was an edge to remove, false otherwise.
   */
  bool removeEdge(const CgNode& parentNode, const CgNode& childNode);

  /**
   * Removes the edge between two nodes of the given names.
   * Does nothing if there are multiple or no nodes with this name, or if there is no such edge.
   * @param callerName
   * @param calleeName
   * @return True if the edges was removed, false otherwise.
   */
  bool removeEdge(const std::string& callerName, const std::string& calleeName);

  /**
   * Inserts a node with the given properties into the call graph.
   * @param function
   * @param origin
   * @param isVirtual
   * @param hasBody
   * @return A reference to the newly created node.
   */
  CgNode& insert(const std::string& function, std::optional<std::string> origin = {}, bool isVirtual = false,
                 bool hasBody = false);

  /**
   * Returns the first node matching the given name.\n
   * If no node exists yet, it creates a new one.
   * @param name to identify the node by
   * @return Reference to the identified node
   */
  CgNode& getOrInsertNode(const std::string& function, std::optional<std::string> origin = {}, bool isVirtual = false,
                          bool hasBody = false);

  /**
   * Erases the node with the given ID. If there are edges from or to this node, they are removed.
   * Note that if there is metadata in other nodes referencing the erased node, this metadata has to be updated
   * manually to maintain consistency.
   *
   * @param id ID of the node to erase.
   * @return True, if the node ID was valid and the corresponding node was erased.
   */
  bool erase(NodeId id);

  /**
   * Merges the given call graph into this one.
   * The other call graph remains unchanged.
   *
   * Merging is performed in four major steps:
   * 1. For each node in the source graph, the policy determines if there is matching node in the target graph. If yes,
   *    it emits an action to either keep or replace the existing node with the source node. Otherwise, the node is
   *    marked to be copied.
   * 2. Nodes with no match are copied inserted into the destination graph, and core attributes of merged nodes are
   *    modified according to the chosen action.
   *    After this step, node references in the edges and metadata are invalid and need to be updated.
   * 3. New edges from the source graph are inserted, using the updated node IDs.
   * 4. Metadata is merged, taking into account the change of node IDs and the performed merge actions.
   *
   * @param other The call graph to merge.
   * @param policy The merge policy.
   */
  MergeRecorder merge(const Callgraph& other, const MergePolicy& policy);

  /**
   * Clears the graph to an empty graph with no main node.
   */
  void clear();

  /**
   * @brief hasNode checks whether a node for #name exists in the graph mapping
   * @param name
   * @return true iff exists, false otherwise
   */
  bool hasNode(const std::string& name) const;

  /**
   * @brief hasNode Checks whether a node with the given ID exists.
   * @param id The node ID.
   * @return true iff exists, false otherwise
   */
  bool hasNode(NodeId id) const;

  /**
   * Checks whether the given node is contained in this graph.
   * @param node
   * @return True iff the node is contained in this graph, false otherwise.
   */
  bool hasNode(const CgNode& node) const;

  /**
   * Counts the number of nodes that match the given name.
   * @param name Name of the function.
   * @return The number of name matches.
   */
  unsigned countNodes(const std::string& name) const;

  /**
   * Checks if there is more than one node with the same function name.
   * @return true if there are duplicate names
   */
  bool hasDuplicateNames() const { return hasDuplicates; }

  /**
   * Returns one of the nodes with given name.
   * @param name
   * @return The first node that matches the name, or null if no node can be found.
   */
  CgNode* getFirstNode(const std::string& name) const;

  /**
   * Like `getFirstNode` but fails if there is not exactly one node with this name.
   * @param name
   * @return The node matching the given name.
   */
  CgNode& getSingleNode(const std::string& name) const;

  /**
   * Returns a list of nodes that match the given name.
   * @param name Name of the function.
   * @return A vector of matching node IDs.
   */
  const NodeList& getNodes(const std::string& name) const;

  /**
   * Looks up the node matching the given ID.
   * @param id
   * @return The node or null, if no such node exists.
   */
  CgNode* getNode(NodeId id) const;

  bool existsEdge(const CgNode& source, const CgNode& target) const;
  bool existsEdge(NodeId source, NodeId target) const;

  /**
   *
   * @param source
   * @param target
   * @return True if there is an edge from any node with name `source` to any node with name `target`.
   */
  bool existsAnyEdge(const std::string& source, const std::string& target) const;

  /**
   * Looks up the set of callees for the given node.
   * @param node
   * @return A #CgNodeRawPtrUSet of callee nodes.
   */
  CgNodeRawPtrUSet getCallees(const CgNode& node) const;

  /**
   * Looks up the set of callees for the given node ID.
   * @param id
   * @return A #CgNodeRawPtrUSet of callee nodes.
   */
  CgNodeRawPtrUSet getCallees(NodeId id) const;

  /**
   * Looks up the set of callers for the given node.
   * @param node
   * @return A #CgNodeRawPtrUSet of callee nodes.
   */
  CgNodeRawPtrUSet getCallers(const CgNode& node) const;

  /**
   * Looks up the set of callers for the given node.
   * @param node
   * @return A #CgNodeRawPtrUSet of caller nodes.
   */
  CgNodeRawPtrUSet getCallers(NodeId id) const;

  /**
   * Returns the number of inserted nodes. Note that this includes erased nodes.
   * @return
   */
  size_t size() const;

  /**
   * Returns the number of nodes, excluding erased nodes.
   * @return
   */
  size_t getNodeCount() const;

  /**
   * Checks if this graph is empty.
   * @return True if there are no active nodes, false otherwise.
   */
  bool isEmpty() const;

  /**
   * Provides access to the raw list of nodes.
   * @return A vector containing all node pointers.
   */
  const NodeContainer& getNodes() const;

  /**
   * Provides access to the raw edge map.
   * @return A map containing node pairs and corresponding metadata.
   */
  const EdgeContainer& getEdges() const;

  /**
   * Attaches edge metadata of type #T to the edge (`func1`, `func2`).
   * Does nothing if the edge does not exist.
   * @tparam T
   * @param func1
   * @param func2
   * @param md
   * @return True if the metadata was successfully attached.
   */
  template <class T>
  bool addEdgeMetaData(const CgNode& func1, const CgNode& func2, std::unique_ptr<T>&& md) {
    if (!hasNode(func1) || !hasNode(func2)) {
      return false;
    }
    return addEdgeMetaData({func1.getId(), func2.getId()}, std::move(md));
  }

  /**
   * Attaches edge metadata of type #T to the edge defined by the given ID pair.
   * Does nothing if the edge does not exist.
   * @tparam T
   * @param id A pair of `NodeId`s
   * @param md
   * @return True if the metadata was successfully attached.
   */
  template <class T>
  bool addEdgeMetaData(const std::pair<NodeId, NodeId> id, std::unique_ptr<T>&& md) {
    if (auto it = edges.find(id); it != edges.end()) {
      it->second[T::key] = std::move(md);
      return true;
    }
    return false;
  }

  /**
   * Returns metadata of this name for the given edge.
   * @param ids Node IDs of the edge pair
   * @param metadataName Name of the metadata
   * @return The metadata or null, if there is none registered with that key.
   */
  MetaData* getEdgeMetaData(const CgNode& func1, const CgNode& func2, const std::string& metadataName) const;

  /**
   * Returns metadata of this name for the given edge.
   * @param ids Node IDs of the edge pair
   * @param metadataName Name of the metadata
   * @return The metadata or null, if there is none registered with that key.
   */
  MetaData* getEdgeMetaData(std::pair<NodeId, NodeId> id, const std::string& metadataName) const;

  /**
   * Returns metadata of type #T for the given edge.
   * @tparam T
   * @param func1
   * @param func2
   * @return The metadata or null, if there is none registered with that type.
   */
  template <class T>
  T* getEdgeMetaData(const CgNode& func1, const CgNode& func2) {
    return getEdgeMetaData<T>({func1.getId(), func2.getId()});
  }

  /**
   * Returns metadata of type #T for the given edge.
   * @tparam T
   * @param id
   * @return The metadata or null, if there is none registered with that type.
   */
  template <class T>
  T* getEdgeMetaData(const std::pair<NodeId, NodeId> id) {
    return static_cast<T>(edges.at(id).at(T::key));
  }

  /**
   * Returns a map of available metadata for this edge. The caller must ensure that this edge exists.
   * @param func1
   * @param func2
   * @return The metadata map.
   */
  const NamedMetadata& getAllEdgeMetaData(const CgNode& func1, const CgNode& func2) const;

  /**
   * Returns a map of available metadata for this edge. The caller must ensure that this edge exists.
   * @param id
   * @return The metadata map.
   */
  const NamedMetadata& getAllEdgeMetaData(const std::pair<NodeId, NodeId> id) const;

  /**
   * Checks if this edge has metadata matching the given name.
   * @param func1
   * @param func2
   * @param metadataName
   * @return True if metadata with this key exists.
   */
  bool hasEdgeMetaData(const CgNode& func1, const CgNode& func2, const std::string& metadataName) const;

  /**
   * Checks if this edge has metadata matching the given name.
   * @param id
   * @param metadataName
   * @return True if metadata with this key exists.
   */
  bool hasEdgeMetaData(const std::pair<NodeId, NodeId> id, const std::string& metadataName) const;

  /**
   * Checks if this edge has metadata of the given type #T.
   * @tparam T
   * @param func1
   * @param func2
   * @return True if metadata of this type exists.
   */
  template <class T>
  bool hasEdgeMetaData(const CgNode& func1, const CgNode& func2) const {
    return hasNode(func1) && hasNode(func2) && hasEdgeMetaData<T>({func1.getId(), func2.getId()});
  }

  /**
   * Checks if this edge has metadata of the given type #T.
   * @tparam T
   * @param id
   * @return True if metadata of this type exists.
   */
  template <class T>
  bool hasEdgeMetaData(const std::pair<NodeId, NodeId> id) const {
    return edges.at(id).find(T::key) != edges.at(id).end();
  }

 private:
  bool addEdgeInternal(NodeId caller, NodeId callee);

 private:
  // this set represents the call graph during the actual computation
  NodeContainer nodes;
  NameIdMap nameIdMap;

  EdgeContainer edges;
  CallerList callerList;
  CalleeList calleeList;

  // Dedicated node pointer to main function
  mutable CgNode* mainNode = nullptr;
  // Tracks if there is more than one node with the same function name
  bool hasDuplicates{false};
  // Tracks number of erased nodes
  size_t numErased{0};
};

}  // namespace metacg

#endif

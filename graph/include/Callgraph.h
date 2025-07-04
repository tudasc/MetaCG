/**
 * File: Callgraph.h
 * License: Part of the MetaCG project. Licensed under BSD 3 clause license. See LICENSE.txt file at
 * https://github.com/tudasc/metacg/LICENSE.txt
 */
#ifndef METACG_GRAPH_CALLGRAPH_H
#define METACG_GRAPH_CALLGRAPH_H

#include "CgNode.h"
#include "Util.h"
#include "MergePolicy.h"

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

class Callgraph {
 public:
  // TODO: Can NodeContainer be a set if nameIdMap maps to CgNodePtr?
  using NodeContainer = std::vector<CgNodePtr>;
  using NodeList = std::vector<NodeId>;

  using NameIdMap = std::unordered_map<std::string, NodeList>;

  using NamedMetadata = std::unordered_map<std::string, std::unique_ptr<MetaData>>;
  using EdgeContainer = std::unordered_map<std::pair<NodeId, NodeId>, NamedMetadata>;
  using CallerList = std::unordered_map<NodeId, NodeList>;
  using CalleeList = std::unordered_map<NodeId, NodeList>;

  Callgraph()
      : nodes(),
        nameIdMap(),
        edges(),
        mainNode(nullptr) {}

  /**
   * Destructor. Prints call graph stats.
   */
  ~Callgraph() = default;

  Callgraph(const Callgraph& other) = delete;             // No copy constructor
  Callgraph& operator=(const Callgraph& other) = delete;  // No copy assign constructor

  Callgraph(Callgraph&& other) = default;             // Use default move constructor
  Callgraph& operator=(Callgraph&& other) = default;  // Use default move assign constructor

  /**
   * @brief getMain
   * @return main function CgNodePtr
   */
  CgNode* getMain();

  /**
   * Inserts an edge from parentNode to childNode
   * @param parentNode function node of calling function
   * @param childNode function node of called function
   */
  void addEdge(const CgNode& parentNode, const CgNode& childNode);

  bool addEdge(NodeId parentID, NodeId childID);

  /**
   * Inserts an edge between the two nodes with the given names.
   * Does nothing if there are multiple or no nodes with this name.
   * @param callerName
   * @param calleeName
   * @return True if the edges was inserted, false otherwise.
   */
  bool addEdge(const std::string& callerName, const std::string& calleeName);

  CgNode& insert(std::string function, std::optional<std::string> origin = {}, bool isVirtual = false,
                 bool hasBody = false);


  /**
   * Returns the first node matching the given name.\n
   * If no node exists yet, it creates a new one.
   * @param name to identify the node by
   * @return Reference to the identified node
   */
  CgNode& getOrInsertNode(std::string function, std::optional<std::string> origin = {}, bool isVirtual = false,
                                          bool hasBody = false);

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
   * @brief hasNode checks whether a node for #name exists in the graph mapping
   * @param name
   * @return true iff exists, false otherwise
   */
  bool hasNode(NodeId id) const;

  unsigned countNodes(const std::string& name) const;

  /**
   * Checks if there is more than one node with the same function name.
   * @return true if there are duplicate names
   */
  bool hasDuplicateNames() const {
    return hasDuplicates;
  }

  /**
   * @brief getNode searches the node in the graph and returns it
   * @param name
   * @return node for function with name - nullptr otherwise
   */
  CgNode* getFirstNode(const std::string& name) const;
  const NodeList& getNodes(const std::string& name) const;

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

  CgNodeRawPtrUSet getCallees(const CgNode& node) const;
  CgNodeRawPtrUSet getCallees(NodeId node) const;

  CgNodeRawPtrUSet getCallers(const CgNode& node) const;
  CgNodeRawPtrUSet getCallers(NodeId node) const;

  size_t size() const;

  bool isEmpty() const;

  const NodeContainer& getNodes() const;
  const EdgeContainer& getEdges() const;

//  void setNodes(NodeContainer external_container);
//  void setEdges(EdgeContainer external_container);
//  void recomputeCache();

  template <class T>
  void addEdgeMetaData(const CgNode& func1, const CgNode& func2, std::unique_ptr<T>&& md) {
    addEdgeMetaData({func1.getId(), func2.getId()}, std::move(md));
  }

  template <class T>
  void addEdgeMetaData(const std::pair<NodeId, NodeId> id, std::unique_ptr<T>&& md) {
    edges.at(id)[T::key] = std::move(md);
  }

  MetaData* getEdgeMetaData(const CgNode& func1, const CgNode& func2, const std::string& metadataName) const;
  MetaData* getEdgeMetaData(std::pair<NodeId, NodeId> id, const std::string& metadataName) const;

  template <class T>
  T* getEdgeMetaData(const CgNode& func1, const CgNode& func2) {
    return getEdgeMetaData<T>({func1.getId(), func2.getId()});
  }

  template <class T>
  T* getEdgeMetaData(const std::pair<NodeId, NodeId> id) {
    return static_cast<T>(edges.at(id).at(T::key));
  }

  const NamedMetadata& getAllEdgeMetaData(const CgNode& func1, const CgNode& func2) const;
  const NamedMetadata& getAllEdgeMetaData(const std::pair<NodeId, NodeId> id) const;

  bool hasEdgeMetaData(const CgNode& func1, const CgNode& func2, const std::string& metadataName) const;
  bool hasEdgeMetaData(const std::pair<NodeId, NodeId> id, const std::string& metadataName) const;

  template <class T>
  bool hasEdgeMetaData(const CgNode& func1, const CgNode& func2) const {
    return hasEdgeMetaData<T>({func1.getId(), func2.getId()});
  }
  template <class T>
  bool hasEdgeMetaData(const std::pair<NodeId, NodeId> id) const {
    return edges.at(id).find(T::key) != edges.at(id).end();
  }

 private:
  // this set represents the call graph during the actual computation
  NodeContainer nodes;
  NameIdMap nameIdMap;

  EdgeContainer edges;
  CallerList callerList;
  CalleeList calleeList;

  // Dedicated node pointer to main function
  CgNode* mainNode = nullptr;
  // Tracks if there is more than one node with the same function name
  bool hasDuplicates{false};

};




//[[maybe_unused]] static Callgraph& getEmptyGraph() {
//  static Callgraph graph;
//  return graph;
//}
}  // namespace metacg

//namespace {
//// A node cannot serialize/deserialize itself, as it is dependent on the containing call graph to assign IDs.
//// Therefore, we have to this manually.
//  metacg::CgNode* createNodeFromJson(metacg::Callgraph& cg, const nlohmann::json& j) {
//    if (j.is_null()) {
//      return nullptr;
//    }
//    std::optional<std::string> origin{};
//    if (j.contains("origin") && !j.at("origin").is_null()) {
//      origin = j.at("origin");
//    }
//    auto& cgNode = cg.insert(j.at("functionName"), origin);
//    assert(j.contains("hasBody") && "Valid node must contain hasBody field");
//    cgNode.setHasBody(j.at("hasBody").get<bool>());
//    cgNode.setMetaDataContainer(j.at("meta"));
//    return &cgNode;
//  }
//
//
//}
//
//namespace nlohmann {
//template <>
//struct adl_serializer<metacg::Callgraph> {
//  // note: the return type is no longer 'void', and the method only takes
//  // one argument
//  static metacg::Callgraph from_json(const json& j) {
//    metacg::Callgraph cg;
//    if (j.is_null())
//      return cg;
//
//    cg.setNodes(j.at("nodes").get<metacg::Callgraph::NodeContainer>());
//    for (auto& entry : j.at("nodes")) {
//      auto* node = createNodeFromJson(cg, entry);
//      // TODO: Cache ID used in format in order to resolve edges
//    }
//    cg.setEdges(j.at("edges").get<metacg::Callgraph::EdgeContainer>());
//    cg.recomputeCache();
//    return cg;
//  }
//
//  // Here's the catch! You must provide a to_json method! Otherwise, you
//  // will not be able to convert move_only_type to json, since you fully
//  // specialized adl_serializer on that type
//  static void to_json(json& j, const metacg::Callgraph& cg) {
//    nlohmann::json e = cg.getEdges();
//    nlohmann::json n = cg.getNodes();
//    j = {{"edges", e}, {"nodes", n}};
//  }
//};
//}  // namespace nlohmann

#endif

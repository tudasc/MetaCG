/**
 * File: Callgraph.cpp
 * License: Part of the MetaCG project. Licensed under BSD 3 clause license. See LICENSE.txt file at
 * https://github.com/tudasc/metacg/LICENSE.txt
 */
#include "Callgraph.h"

#include "LoggerUtil.h"
#include "metadata/OverrideMD.h"

#include <string>

int metacg_RegistryInstanceCounter{0};

using namespace metacg;

CgNode* Callgraph::getMain() {
  if (mainNode) {
    return mainNode;
  }

  if ((mainNode = getFirstNode("main")) || (mainNode = getFirstNode("_Z4main")) || (mainNode = getFirstNode("_ZSt4mainiPPc"))) {
    return mainNode;
  }

  return nullptr;
}

CgNode& Callgraph::insert(std::string function, std::optional<std::string> origin, bool isVirtual,
               bool hasBody) {
  NodeId id = nodes.size(); // TODO: Improve this
  // Note: Can't use make_unique here because make_unqiue is not (and should not be) a friend of the CgNode constructor.
  nodes.emplace_back(new CgNode(id, function, origin, isVirtual, hasBody));
  auto& nodesWithName = nameIdMap[function];
  if (nodesWithName.size() > 0) {
    hasDuplicates = true;
  }
  nodesWithName.push_back(id);
  return *nodes.back();
}

CgNode& Callgraph::getOrInsertNode(std::string function, std::optional<std::string> origin, bool isVirtual,
                                        bool hasBody) {
  auto* node = getFirstNode(function);
  if (node) {
    return *node;
  }
  return insert(std::move(function), std::move(origin), isVirtual, hasBody);
}

void Callgraph::clear() {
  nodes.clear();
  nameIdMap.clear();
  edges.clear();
  callerList.clear();
  calleeList.clear();
  mainNode = nullptr;
}

void Callgraph::addEdge(const CgNode& parentNode, const CgNode& childNode) {
  if (edges.find({parentNode.getId(), childNode.getId()}) != edges.end()) {
    MCGLogger::instance().getErrConsole()->warn("Edge between {} and {} already exist: Skipping edge insertion",
                                                parentNode.getFunctionName(), childNode.getFunctionName());
    return;
  }
  calleeList[parentNode.getId()].push_back(childNode.getId());
  callerList[childNode.getId()].push_back(parentNode.getId());
  NamedMetadata edgeMd;
  edges[{parentNode.getId(), childNode.getId()}] = std::move(edgeMd);
}

bool Callgraph::addEdge(NodeId parentID, NodeId childID) {
  auto* parentNode = getNode(parentID);
  if (!parentNode) {
    MCGLogger::instance().getErrConsole()->error("Source ID {} does not exist in graph",
                                                 parentID);
    return false;
  }
  auto* childNode = getNode(childID);
  if (!childNode) {
    MCGLogger::instance().getErrConsole()->error("Target ID {} does not exist in graph",
                                                 childID);
    return false;
  }
  addEdge(*parentNode, *childNode);
  return true;
}

bool Callgraph::addEdge(const std::string& callerName, const std::string& calleeName) {
  auto& callerMatches = getNodes(callerName);
  auto& calleeMatches = getNodes(calleeName);
  if (callerMatches.size() != 1 || callerMatches.size() != 1) {
    return false;
  }
  addEdge(callerMatches.front(), calleeMatches.front());
  return true;
}


bool Callgraph::hasNode(const std::string& name) const {
  auto it = nameIdMap.find(name);
  return it != nameIdMap.end();
}

bool Callgraph::hasNode(NodeId id) const {
  return id < nodes.size() && nodes.at(id);
}

unsigned Callgraph::countNodes(const std::string& name) const {
  return nameIdMap.count(name);
}

CgNode* Callgraph::getFirstNode(const std::string& name) const {
  if (auto it = nameIdMap.find(name); it != nameIdMap.end()) {
    if (it->second.empty()) {
      return nullptr;
    }
    return getNode(it->second.front());
  }
  return nullptr;
}

const Callgraph::NodeList& Callgraph::getNodes(const std::string& name) const {
  if (auto it = nameIdMap.find(name); it != nameIdMap.end()) {
    return it->second;
  }
  static const Callgraph::NodeList empty{};
  return empty;
}

CgNode* Callgraph::getNode(NodeId id) const {
  if (id > nodes.size()) {
    return nullptr;
  }
  return nodes.at(id).get();
}

size_t Callgraph::size() const { return nodes.size(); }

bool Callgraph::isEmpty() const { return nodes.empty(); }

MergeRecorder Callgraph::merge(const metacg::Callgraph& other, const metacg::MergePolicy& policy) {
//  // TODO: Introduce merge policies. For now, we fall back on the old behavior of merging by name.
//  // Lambda function to clone nodes from the other call graph
//  std::function<void(metacg::Callgraph*, const metacg::Callgraph&, metacg::CgNode*)> copyNode =
//      [&](metacg::Callgraph* destination, const metacg::Callgraph& source, metacg::CgNode* node) {
//        std::string functionName = node->getFunctionName();
//        metacg::CgNode& mergeNode = destination->getOrInsertNode(functionName, node->getOrigin());
//
//        if (node->getHasBody()) {
//          auto callees = source.getCallees(*node);
//
//          for (auto* c : callees) {
//            std::string calleeName = c->getFunctionName();
//            if (!destination->hasNode(calleeName)) {
//              copyNode(destination, source, c);
//            }
//            auto& calleeNode = *destination->getFirstNode(c->getFunctionName());
//
//            if (!destination->existsEdge(mergeNode, calleeNode)) {
//              destination->addEdge(mergeNode, calleeNode);
//            }
//          }
//
//          mergeNode.setHasBody(node->getHasBody());
//
//          if (!mergeNode.has<OverrideMD>() && node->has<OverrideMD>()) {
//              mergeNode.addMetaData<OverrideMD>();
//          }
//        }
//
//        for (const auto& it : node->getMetaDataContainer()) {
//          if (mergeNode.has(it.first)) {
//            mergeNode.get(it.first)->merge(*(it.second));
//          } else {
//            mergeNode.addMetaData(it.second->clone());
//          }
//        }
//      };
//
//  auto cloneNode = [&](CgNode& sourceNode) {
//
//  };

  // Records performed merge actions to enable proper updating node references (in edges and metadata).
  MergeRecorder recorder;


  // Step 1 & 2: iterate over all nodes, determine matches according to the policy, and merge the nodes into this graph.
  for (auto& node: other.nodes) {
    auto match = policy.findMatchingNode(*this, *node);
    if (match) {
      auto& action = match.value();
      auto targetNode = this->getNode(action.targetNode);
      assert(targetNode && "Target node must not be null");
      // Perform the merge
      if (action.replace) {
        // Replace the core attributes with those from the source node.
        targetNode->setFunctionName(node->getFunctionName());
        targetNode->setHasBody(node->getHasBody());
        targetNode->setOrigin(node->getOrigin());
      } else {
        // Nothing to be done - we keep the target node.
      }
      // Record the action
      recorder.recordMerge(node->getId(), action);
    } else {
      // Creating a new node (ignoring edges and metadata for now).
      // Setting isVirtual to false initially, as metadata is copied over later anyway.
      auto& targetNode = this->insert(node->getFunctionName(), node->getOrigin(), false, node->hasBody);
      recorder.recordCopy(node->getId(), targetNode.getId());
    }
  }

  // IDs are finalized at this point, retrieve the mapping.
  auto& mapping = recorder.getMapping();

  // Step 3: Update edges. This involves inserting edges from the source graph and mapping them to the correct node IDs.
  //         Note that there is no need to update existing nodes in the destination graph, as the IDs remain unchanged.
  for (auto& edge : other.getEdges()) {
    auto& sourceIds = edge.first;
    assert(mapping.count(sourceIds.first) == 1 && mapping.count(sourceIds.second) == 1 && "All nodes have to be recorded at this point");
    auto mappedCallerId = mapping.at(sourceIds.first);
    auto mappedCalleeId = mapping.at(sourceIds.second);
    if (!this->existsEdge(mappedCallerId, mappedCalleeId)) {
      // Edge does not yet exist -> insert
      this->addEdge(mappedCallerId, mappedCalleeId);
    }
    // Merge edge metadata
    for (auto& edgeMd : other.getAllEdgeMetaData(edge.first)) {
      // Check if this metadata already exists
      if (auto* md = this->getEdgeMetaData({mappedCallerId, mappedCalleeId}, edgeMd.first); md) {
        auto action = recorder.getAction(sourceIds.first);
        assert(action && "Metadata should not exists without a merge action");
        md->merge(*edgeMd.second, *action, mapping);
      } else {
        auto clonedMd = edgeMd.second->clone();
        clonedMd->applyMapping(mapping);
        this->addEdgeMetaData({mappedCallerId, mappedCalleeId}, std::move(clonedMd));
      }
      // TODO: Mapping needed in edge metadata?
    }
  }

  // Step 4: Copy or merge node metadata as needed.
  for (auto& node: other.nodes) {
    assert(mapping.count(node->getId()) == 1 && "All nodes have to be recorded at this point");
    auto mappedNodeId = mapping.at(node->getId());
    auto targetNode = this->getNode(mappedNodeId);
    assert(targetNode && "Mapped node ID must be valid");

    for (auto& md : node->getMetaDataContainer()) {
      if (targetNode->has(md.first)) {
        auto action = recorder.getAction(node->getId());
        assert(action && "Metadata must not exist without previous merge action");
        targetNode->get(md.first)->merge(*(md.second), *action, mapping);
      } else {
        auto clonedMd = md.second->clone();
        clonedMd->applyMapping(mapping);
        targetNode->addMetaData(std::move(clonedMd));
      }
    }
  }

  return recorder;
}

const metacg::Callgraph::NodeContainer& Callgraph::getNodes() const { return nodes; }

const metacg::Callgraph::EdgeContainer& Callgraph::getEdges() const { return edges; }

bool Callgraph::existsEdge(const CgNode& source, const CgNode& target) const {
  return existsEdge(source.getId(), target.getId());
}

bool Callgraph::existsEdge(NodeId source, NodeId target) const {
  return edges.find({source, target}) != edges.end();
}

bool Callgraph::existsAnyEdge(const std::string& source, const std::string& target) const {
  auto& sourceNodes = getNodes(source);
  auto& targetNodes = getNodes(target);
  for (auto sourceId: sourceNodes) {
    for (auto targetId : targetNodes) {
      if (existsEdge(sourceId, targetId)) {
        return true;
      }
    }
  }
  return false;
}

CgNodeRawPtrUSet Callgraph::getCallees(const CgNode& node) const { return getCallees(node.getId()); }
CgNodeRawPtrUSet Callgraph::getCallees(NodeId node) const {
  CgNodeRawPtrUSet returnSet;
  if (calleeList.count(node) == 0) {
    return returnSet;
  }
  returnSet.reserve(calleeList.at(node).size());
  for (auto elem : calleeList.at(node)) {
    returnSet.insert(nodes.at(elem).get());
  }
  return returnSet;
}

CgNodeRawPtrUSet Callgraph::getCallers(const CgNode& node) const { return getCallers(node.getId()); };
CgNodeRawPtrUSet Callgraph::getCallers(NodeId node) const {
  CgNodeRawPtrUSet returnSet;
  if (callerList.count(node) == 0) {
    return returnSet;
  }
  returnSet.reserve(callerList.at(node).size());
  for (auto elem : callerList.at(node)) {
    returnSet.insert(nodes.at(elem).get());
  }
  return returnSet;
}

// TODO: When is this needed?
//void Callgraph::setNodes(Callgraph::NodeContainer external_container) { nodes = std::move(external_container); }
//void Callgraph::setEdges(Callgraph::EdgeContainer external_container) { edges = std::move(external_container); }
//
//void Callgraph::recomputeCache() {
//  nameIdMap.clear();
//  for (const auto& elem : nodes) {
//    nameIdMap[elem->getFunctionName()].push_back(elem->getId());
//  }
//  // TODO: What's the benefit of maintaining a separate caller/callee map?
//  callerList.clear();
//  calleeList.clear();
//  for (const auto& elem : edges) {
//    calleeList[elem.first.first].push_back(elem.first.second);
//    callerList[elem.first.second].push_back(elem.first.first);
//  }
//}

MetaData* Callgraph::getEdgeMetaData(const CgNode& func1, const CgNode& func2, const std::string& metadataName) const {
  return getEdgeMetaData({func1.getId(), func2.getId()}, metadataName);
}

/**
 * Returns metadata for the given edge. The edge is assumed to exists.
 * @param ids Node IDs of the edge pair
 * @param metadataName Name of the metadata
 * @return The metadata or null, if there is none registered with that key.
 */
MetaData* Callgraph::getEdgeMetaData(std::pair<size_t, size_t> ids, const std::string& metadataName) const {
  assert(edges.find(ids) != edges.end() && "Edge does not exist");
  auto& edgeMD = edges.at(ids);
  if (auto it = edgeMD.find(metadataName); it != edgeMD.end()) {
    return it->second.get();
  }
  return nullptr;
}

const metacg::Callgraph::NamedMetadata& Callgraph::getAllEdgeMetaData(const CgNode& func1, const CgNode& func2) const {
  return getAllEdgeMetaData({func1.getId(), func2.getId()});
}

const metacg::Callgraph::NamedMetadata& Callgraph::getAllEdgeMetaData(const std::pair<size_t, size_t> id) const {
  return edges.at(id);
}

bool Callgraph::hasEdgeMetaData(const CgNode& func1, const CgNode& func2, const std::string& metadataName) const {
  return hasEdgeMetaData({func1.getId(), func2.getId()}, metadataName);
}

bool Callgraph::hasEdgeMetaData(const std::pair<size_t, size_t> id, const std::string& metadataName) const {
  return edges.at(id).find(metadataName) != edges.at(id).end();
}


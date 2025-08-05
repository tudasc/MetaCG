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

  if ((mainNode = getFirstNode("main")) || (mainNode = getFirstNode("_Z4main")) ||
      (mainNode = getFirstNode("_ZSt4mainiPPc"))) {
    return mainNode;
  }

  return nullptr;
}

CgNode& Callgraph::insert(const std::string& function, std::optional<std::string> origin, bool isVirtual,
                          bool hasBody) {
  NodeId id = nodes.size();
  // Note: Can't use make_unique here because make_unqiue is not (and should not be) a friend of the CgNode constructor.
  nodes.emplace_back(new CgNode(id, function, std::move(origin), isVirtual, hasBody));
  auto& nodesWithName = nameIdMap[function];
  if (!nodesWithName.empty()) {
    hasDuplicates = true;
  }
  nodesWithName.push_back(id);
  return *nodes.back();
}

bool Callgraph::erase(NodeId id) {
  if (!hasNode(id)) {
    return false;
  }
  // Remove edges
  for (auto& calleeId : calleeList[id]) {
    edges.erase({id, calleeId});
  }
  calleeList.erase(id);
  for (auto& callerId : callerList[id]) {
    edges.erase({callerId, id});
  }
  callerList.erase(id);
  // Destroy the node
  auto& ptr = nodes[id];
  assert(ptr && "The ID must correspond to a valid node");
  std::string name = ptr->getFunctionName();
  nameIdMap[name].erase(std::find(nameIdMap[name].begin(), nameIdMap[name].end(), id));
  ptr.reset();
  numErased++;
  return true;
}

CgNode& Callgraph::getOrInsertNode(const std::string& function, std::optional<std::string> origin, bool isVirtual,
                                   bool hasBody) {
  auto* node = getFirstNode(function);
  if (node) {
    return *node;
  }
  return insert(function, std::move(origin), isVirtual, hasBody);
}

void Callgraph::clear() {
  nodes.clear();
  nameIdMap.clear();
  edges.clear();
  callerList.clear();
  calleeList.clear();
  mainNode = nullptr;
  numErased = 0;
  hasDuplicates = false;
}

bool Callgraph::addEdgeInternal(NodeId caller, NodeId callee) {
  if (edges.find({caller, callee}) != edges.end()) {
    MCGLogger::instance().getErrConsole()->warn("Edge between {} and {} already exist: Skipping edge insertion",
                                                getNode(caller)->getFunctionName(), getNode(callee)->getFunctionName());
    return false;
  }
  calleeList[caller].push_back(callee);
  callerList[callee].push_back(caller);
  NamedMetadata edgeMd;
  edges[{caller, callee}] = std::move(edgeMd);
  return true;
}

bool Callgraph::addEdge(const CgNode& parentNode, const CgNode& childNode) {
  if (!hasNode(parentNode) || !hasNode(childNode)) {
    return false;
  }
  return addEdgeInternal(parentNode.getId(), childNode.getId());
}

bool Callgraph::addEdge(NodeId parentID, NodeId childID) {
  auto* parentNode = getNode(parentID);
  if (!parentNode) {
    MCGLogger::instance().getErrConsole()->error("Source ID {} does not exist in graph", parentID);
    return false;
  }
  auto* childNode = getNode(childID);
  if (!childNode) {
    MCGLogger::instance().getErrConsole()->error("Target ID {} does not exist in graph", childID);
    return false;
  }
  return addEdgeInternal(parentID, childID);
}

bool Callgraph::addEdge(const std::string& callerName, const std::string& calleeName) {
  auto& callerMatches = getNodes(callerName);
  auto& calleeMatches = getNodes(calleeName);
  if (callerMatches.size() != 1 || calleeMatches.size() != 1) {
    return false;
  }
  addEdge(callerMatches.front(), calleeMatches.front());
  return true;
}

bool Callgraph::removeEdge(NodeId parentID, NodeId childID) {
  bool existed = edges.erase({parentID, childID});
  if (existed) {
    auto& parentCallees = calleeList[parentID];
    auto calleeEntry = std::find(parentCallees.begin(), parentCallees.end(), childID);
    parentCallees.erase(calleeEntry);
    auto& childCallers = callerList[childID];
    auto callerEntry = std::find(childCallers.begin(), childCallers.end(), parentID);
    childCallers.erase(callerEntry);
  }
  return existed;
}

bool Callgraph::removeEdge(const CgNode& parentNode, const CgNode& childNode) {
  if (!hasNode(parentNode) && !hasNode(childNode)) {
    return false;
  }
  return removeEdge(parentNode.id, childNode.id);
}

bool Callgraph::removeEdge(const std::string& callerName, const std::string& calleeName) {
  auto& callerMatches = getNodes(callerName);
  auto& calleeMatches = getNodes(calleeName);
  if (callerMatches.size() != 1 || calleeMatches.size() != 1) {
    return false;
  }
  return removeEdge(callerMatches.front(), calleeMatches.front());
}

bool Callgraph::hasNode(const std::string& name) const {
  auto it = nameIdMap.find(name);
  if (it != nameIdMap.end()) {
    return std::any_of(it->second.begin(), it->second.end(), [&](auto& id) { return hasNode(id); });
  }
  return false;
}

bool Callgraph::hasNode(NodeId id) const { return id < nodes.size() && nodes.at(id); }

bool Callgraph::hasNode(const CgNode& node) const {
  if (node.id >= nodes.size()) {
    return false;
  }
  auto& nodeAtId = nodes.at(node.id);
  // Check for matching address
  return nodeAtId && nodeAtId.get() == &node;
}

unsigned Callgraph::countNodes(const std::string& name) const { return nameIdMap.count(name); }

CgNode* Callgraph::getFirstNode(const std::string& name) const {
  if (auto it = nameIdMap.find(name); it != nameIdMap.end()) {
    if (it->second.empty()) {
      return nullptr;
    }
    return getNode(it->second.front());
  }
  return nullptr;
}

CgNode& Callgraph::getSingleNode(const std::string& name) const {
  auto it = nameIdMap.find(name);
  assert(it->second.size() == 1 && "There must be exactly one node with this name");
  auto* node = getNode(it->second.front());
  assert(node && "This node should always exist");
  return *node;
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

size_t Callgraph::getNodeCount() const { return nodes.size() - numErased; }

bool Callgraph::isEmpty() const { return getNodeCount() == 0; }

MergeRecorder Callgraph::merge(const metacg::Callgraph& other, const metacg::MergePolicy& policy) {
  // Records performed merge actions to enable properly updating node references (in edges and metadata).
  MergeRecorder recorder;

  // Step 1 & 2: iterate over all nodes, determine actions according to the policy, and merge the nodes into this graph.
  for (auto& node : other.nodes) {
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
    assert(mapping.count(sourceIds.first) == 1 && mapping.count(sourceIds.second) == 1 &&
           "All nodes have to be recorded at this point");
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
    }
  }

  // Step 4: Copy or merge node metadata as needed.
  for (auto& node : other.nodes) {
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

  // Step 5: merge global metadata
  for (auto& md : other.getMetaDataContainer()) {
    auto* existingMd = this->get(md.first);
    if (existingMd) {
      // Merge action is irrelevant (node ID set to -1 by default), as there is no associated node.
      existingMd->merge(*(md.second), MergeAction(), mapping);
    } else {
      auto clonedMd = md.second->clone();
      clonedMd->applyMapping(mapping);
      this->addMetaData(std::move(clonedMd));
    }
  }

  return recorder;
}

const metacg::Callgraph::NodeContainer& Callgraph::getNodes() const { return nodes; }

const metacg::Callgraph::EdgeContainer& Callgraph::getEdges() const { return edges; }

bool Callgraph::existsEdge(const CgNode& source, const CgNode& target) const {
  return hasNode(source) && hasNode(target) && existsEdge(source.getId(), target.getId());
}

bool Callgraph::existsEdge(NodeId source, NodeId target) const { return edges.find({source, target}) != edges.end(); }

bool Callgraph::existsAnyEdge(const std::string& source, const std::string& target) const {
  auto& sourceNodes = getNodes(source);
  auto& targetNodes = getNodes(target);
  for (auto sourceId : sourceNodes) {
    for (auto targetId : targetNodes) {
      if (existsEdge(sourceId, targetId)) {
        return true;
      }
    }
  }
  return false;
}

CgNodeRawPtrUSet Callgraph::getCallees(const CgNode& node) const {
  if (hasNode(node)) {
    return getCallees(node.getId());
  }
  return {};
}

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

CgNodeRawPtrUSet Callgraph::getCallers(const CgNode& node) const {
  if (hasNode(node)) {
    return getCallers(node.getId());
  }
  return {};
};

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

MetaData* Callgraph::getEdgeMetaData(const CgNode& func1, const CgNode& func2, const std::string& metadataName) const {
  if (hasNode(func1) && hasNode(func2)) {
    return getEdgeMetaData({func1.getId(), func2.getId()}, metadataName);
  }
  return nullptr;
}

MetaData* Callgraph::getEdgeMetaData(std::pair<size_t, size_t> ids, const std::string& metadataName) const {
  auto edgeIt = edges.find(ids);
  if (edgeIt == edges.end()) {
    return nullptr;
  }
  auto& edgeMD = edgeIt->second;
  if (auto it = edgeMD.find(metadataName); it != edgeMD.end()) {
    return it->second.get();
  }
  return nullptr;
}

const metacg::Callgraph::NamedMetadata& Callgraph::getAllEdgeMetaData(const CgNode& func1, const CgNode& func2) const {
  return getAllEdgeMetaData({func1.getId(), func2.getId()});
}

const metacg::Callgraph::NamedMetadata& Callgraph::getAllEdgeMetaData(const std::pair<size_t, size_t> id) const {
  assert(edges.find(id) != edges.end() && "Edge does not exist");
  return edges.at(id);
}

bool Callgraph::hasEdgeMetaData(const CgNode& func1, const CgNode& func2, const std::string& metadataName) const {
  return hasNode(func1) && hasNode(func2) && hasEdgeMetaData({func1.getId(), func2.getId()}, metadataName);
}

bool Callgraph::hasEdgeMetaData(const std::pair<size_t, size_t> id, const std::string& metadataName) const {
  if (auto it = edges.find(id); it != edges.end()) {
    return it->second.find(metadataName) != it->second.end();
  }
  return false;
}

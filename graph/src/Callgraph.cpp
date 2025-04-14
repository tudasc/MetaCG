/**
 * File: Callgraph.cpp
 * License: Part of the MetaCG project. Licensed under BSD 3 clause license. See LICENSE.txt file at
 * https://github.com/tudasc/metacg/LICENSE.txt
 */
#include "Callgraph.h"
#include "LoggerUtil.h"
#include <string>

int metacg_RegistryInstanceCounter{0};

using namespace metacg;

CgNode* Callgraph::getMain() {
  if (mainNode) {
    return mainNode;
  }

  if ((mainNode = getNode("main")) || (mainNode = getNode("_Z4main")) || (mainNode = getNode("_ZSt4mainiPPc"))) {
    return mainNode;
  }

  return nullptr;
}

size_t Callgraph::insert(const std::string& nodeName, const std::string& origin) {
  return insert(std::make_unique<CgNode>(nodeName, origin));
}

/**
 * This function takes ownership of the passed node,
 * and adds it to the callgraph
 *
 * @param node
 * @return the id of the inserted node
 */
size_t Callgraph::insert(CgNodePtr node) {
  const size_t nodeId = node->getId();
  if (auto n = nameIdMap.find(node->getFunctionName()); n != nameIdMap.end()) {
    if (n->first != node->getFunctionName()) {
      MCGLogger::instance().getErrConsole()->warn(
          "There already exists a mapping from {} to {}, but the newly inserted node {} generates the same ID ({}) "
          "this probably is a hash function collision.",
          n->first, n->second, node->getFunctionName(), node->getId());
      ++nodeHashCollisionCounter;
      if (!empiricalCollisionCounting) {
        MCGLogger::instance().getErrConsole()->error(
            "Collisions are treated as errors. Stopping.\nExport METACG_EMPIRICAL_COLLISION_TRACKING=1 to override.");
        abort();
      }
    } else {
      MCGLogger::instance().getErrConsole()->warn(
          "A Node with ID {} and name {} allready exists in the Map: Skipping insertion into Map", n->second, n->first);
    }
  } else {
    nameIdMap.insert({node->getFunctionName(), node->getId()});
  }
  if (auto n = nodes.find(node->getId()); n != nodes.end()) {
    MCGLogger::instance().getErrConsole()->warn("A Node with ID {} already exists: Skipping insertion",
                                                n->second->getId());
    MCGLogger::instance().getErrConsole()->warn("The Node will be destroyed.");
  } else {
    nodes[nodeId] = std::move(node);
  }

  return nodeId;
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
  edges[{parentNode.getId(), childNode.getId()}] = {};
}

void Callgraph::addEdge(const std::string& parentName, const std::string& childName) {
  if (nameIdMap.find(parentName) == nameIdMap.end()) {
    MCGLogger::instance().getErrConsole()->warn("Source node: {} does not exist in graph: Inserting Node", parentName);
    insert(parentName, "unknownOrigin");
  }
  if (nameIdMap.find(childName) == nameIdMap.end()) {
    MCGLogger::instance().getErrConsole()->warn("Target node: {} does not exist in graph: Inserting Node", childName);
    insert(childName, "unknownOrigin");
  }
  addEdge(nameIdMap[parentName], nameIdMap[childName]);
}

void Callgraph::addEdge(size_t parentID, size_t childID) {
  if (nodes.find(parentID) == nodes.end()) {
    MCGLogger::instance().getErrConsole()->error("Source ID {} does not exist in graph: Unrecoverable graph error",
                                                 parentID);
    abort();
  }
  if (nodes.find(childID) == nodes.end()) {
    MCGLogger::instance().getErrConsole()->error("Target ID {} does not exist in graph: Unrecoverable graph error",
                                                 childID);
    abort();
  }
  addEdge(*nodes.at(parentID), *nodes.at(childID));
}

void Callgraph::addEdge(const CgNode* parent, const CgNode* child) {
  // We assume these pointers to be valid in the context of the callgraph
  addEdge(parent->getId(), child->getId());
}

bool Callgraph::hasNode(const std::string& name) const {
  auto r = nameIdMap.find(name);
  return r != nameIdMap.end();
}

bool Callgraph::hasNode(const CgNode& n) const { return hasNode(n.getId()); }

bool Callgraph::hasNode(const CgNode* n) const { return hasNode(n->getId()); }

bool Callgraph::hasNode(size_t id) const {
  auto r = nodes.find(id);
  return r != nodes.end();
}

CgNode* Callgraph::getNode(const std::string& name) const {
  if (nameIdMap.find(name) == nameIdMap.end()) {
    return nullptr;
  }

  auto nodeId = nameIdMap.at(name);

  if (nodes.find(nodeId) == nodes.end()) {
    return nullptr;
  }

  return nodes.at(nodeId).get();
}

CgNode* Callgraph::getNode(size_t id) const {
  if (nodes.find(id) == nodes.end()) {
    return nullptr;
  }
  return nodes.at(id).get();
}

size_t Callgraph::size() const { return nodes.size(); }

bool Callgraph::isEmpty() const { return nodes.empty(); }

CgNode* Callgraph::getOrInsertNode(const std::string& name, const std::string& origin) {
  if (auto node = getNode(name); node != nullptr) {
    return node;
  }

  auto node_id = insert(name, origin);
  assert(nodes.find(node_id) != nodes.end());
  return nodes[node_id].get();
}

void metacg::Callgraph::merge(const metacg::Callgraph& other) {
  // Lambda function to clone nodes from the other call graph
  std::function<void(metacg::Callgraph*, const metacg::Callgraph&, metacg::CgNode*)> copyNode =
      [&](metacg::Callgraph* destination, const metacg::Callgraph& source, metacg::CgNode* node) {
        std::string functionName = node->getFunctionName();
        metacg::CgNode* mergeNode = destination->getOrInsertNode(functionName, node->getOrigin());

        if (node->getHasBody()) {
          auto callees = source.getCallees(node);

          for (auto* c : callees) {
            std::string calleeName = c->getFunctionName();
            if (!destination->hasNode(calleeName)) {
              copyNode(destination, source, c);
            }

            if (!destination->existEdgeFromTo(functionName, calleeName)) {
              destination->addEdge(functionName, calleeName);
            }
          }

          mergeNode->setHasBody(node->getHasBody());

          if (!mergeNode->has<OverrideMD>() && node->has<OverrideMD>()) {
              mergeNode->addMetaData<OverrideMD>(new OverrideMD());
          }
        }

        for (const auto& it : node->getMetaDataContainer()) {
          if (mergeNode->has(it.first)) {
            mergeNode->get(it.first)->merge(*(it.second));
          } else {
            mergeNode->addMetaData(it.second->clone());
          }
        }
      };

  // Iterate over all nodes and merge them into this graph
  for (auto it = other.nodes.begin(); it != other.nodes.end(); ++it) {
    auto& currentNode = it->second;
    copyNode(this, other, currentNode.get());
  }
}

const metacg::Callgraph::NodeContainer& Callgraph::getNodes() const { return nodes; }

const metacg::Callgraph::EdgeContainer& Callgraph::getEdges() const { return edges; }

bool Callgraph::existEdgeFromTo(const CgNode& source, const CgNode& target) const {
  return existEdgeFromTo(source.getId(), target.getId());
}

bool Callgraph::existEdgeFromTo(const CgNode* source, const CgNode*& target) const {
  return existEdgeFromTo(source->getId(), target->getId());
}

bool Callgraph::existEdgeFromTo(size_t source, size_t target) const {
  return edges.find({source, target}) != edges.end();
}

bool Callgraph::existEdgeFromTo(const std::string& source, const std::string& target) const {
  if (nameIdMap.find(source) == nameIdMap.end() || nameIdMap.find(target) == nameIdMap.end()) {
    return false;
  }

  return existEdgeFromTo(nameIdMap.at(source), nameIdMap.at(target));
}

CgNodeRawPtrUSet Callgraph::getCallees(const CgNode& node) const { return getCallees(node.getId()); }
CgNodeRawPtrUSet Callgraph::getCallees(const CgNode* node) const { return getCallees(node->getId()); }
CgNodeRawPtrUSet Callgraph::getCallees(size_t node) const {
  CgNodeRawPtrUSet returnSet;
  if (calleeList.count(node) == 0) {
    return returnSet;
  }
  returnSet.reserve(calleeList.at(node).size());
  for (auto elem : calleeList.at(node)) {
    assert(nodes.count(elem) == 1);
    returnSet.insert(nodes.at(elem).get());
  }
  return returnSet;
}
CgNodeRawPtrUSet Callgraph::getCallees(const std::string& node) const {
  assert(nameIdMap.find(node) != nameIdMap.end());
  return getCallees(nameIdMap.at(node));
}

CgNodeRawPtrUSet Callgraph::getCallers(const CgNode* node) const { return getCallers(node->getId()); }
CgNodeRawPtrUSet Callgraph::getCallers(const CgNode& node) const { return getCallers(node.getId()); };
CgNodeRawPtrUSet Callgraph::getCallers(size_t node) const {
  CgNodeRawPtrUSet returnSet;
  if (callerList.count(node) == 0) {
    return returnSet;
  }
  returnSet.reserve(callerList.at(node).size());
  for (auto elem : callerList.at(node)) {
    assert(nodes.count(elem) == 1);
    returnSet.insert(nodes.at(elem).get());
  }
  return returnSet;
}
CgNodeRawPtrUSet Callgraph::getCallers(const std::string& node) const {
  assert(nameIdMap.find(node) != nameIdMap.end());
  return getCallers(nameIdMap.at(node));
}

void Callgraph::setNodes(Callgraph::NodeContainer external_container) { nodes = std::move(external_container); }
void Callgraph::setEdges(Callgraph::EdgeContainer external_container) { edges = std::move(external_container); }
void Callgraph::recomputeCache() {
  nameIdMap.clear();
  for (const auto& elem : nodes) {
    nameIdMap[elem.second->getFunctionName()] = elem.first;
  }
  callerList.clear();
  calleeList.clear();
  for (const auto& elem : edges) {
    calleeList[elem.first.first].push_back(elem.first.second);
    callerList[elem.first.second].push_back(elem.first.first);
  }
}

MetaData* Callgraph::getEdgeMetaData(const CgNode& func1, const CgNode& func2, const std::string& metadataName) const {
  return getEdgeMetaData({func1.getId(), func2.getId()}, metadataName);
}
MetaData* Callgraph::getEdgeMetaData(const CgNode* func1, const CgNode* func2, const std::string& metadataName) const {
  return getEdgeMetaData({func1->getId(), func2->getId()}, metadataName);
}
MetaData* Callgraph::getEdgeMetaData(const std::pair<size_t, size_t> id, const std::string& metadataName) const {
  return edges.at(id).at(metadataName);
}
MetaData* Callgraph::getEdgeMetaData(const std::string& func1, const std::string& func2,
                                     const std::string& metadataName) const {
  return getEdgeMetaData({nameIdMap.at(func1), nameIdMap.at(func2)}, metadataName);
}

const metacg::Callgraph::NamedMetadata& Callgraph::getAllEdgeMetaData(const CgNode& func1, const CgNode& func2) const {
  return getAllEdgeMetaData({func1.getId(), func2.getId()});
}
const metacg::Callgraph::NamedMetadata& Callgraph::getAllEdgeMetaData(const CgNode* func1, const CgNode* func2) const {
  return getAllEdgeMetaData({func1->getId(), func2->getId()});
}
const metacg::Callgraph::NamedMetadata& Callgraph::getAllEdgeMetaData(const std::pair<size_t, size_t> id) const {
  return edges.at(id);
}
const metacg::Callgraph::NamedMetadata& Callgraph::getAllEdgeMetaData(const std::string& func1,
                                                                      const std::string& func2) const {
  return getAllEdgeMetaData({nameIdMap.at(func1), nameIdMap.at(func2)});
}

bool Callgraph::hasEdgeMetaData(const CgNode& func1, const CgNode& func2, const std::string& metadataName) const {
  return hasEdgeMetaData({func1.getId(), func2.getId()}, metadataName);
}
bool Callgraph::hasEdgeMetaData(const CgNode* func1, const CgNode* func2, const std::string& metadataName) const {
  return hasEdgeMetaData({func1->getId(), func2->getId()}, metadataName);
}
bool Callgraph::hasEdgeMetaData(const std::pair<size_t, size_t> id, const std::string& metadataName) const {
  return edges.at(id).find(metadataName) != edges.at(id).end();
}
bool Callgraph::hasEdgeMetaData(const std::string& func1, const std::string& func2,
                                const std::string& metadataName) const {
  return hasEdgeMetaData({nameIdMap.at(func1), nameIdMap.at(func2)}, metadataName);
}

void Callgraph::dumpCGStats() const {
  auto console = MCGLogger::instance().getConsole();

  // TODO: Can we use the FMT stuff that comes with spdlog for the to_string here?
  if (empiricalCollisionCounting) {
    console->info(" == Callgraph stats == \n");
    console->info("Node hash collisions: " + std::to_string(nodeHashCollisionCounter));
  }
}

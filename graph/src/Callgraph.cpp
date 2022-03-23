/**
 * File: Callgraph.cpp
 * License: Part of the MetaCG project. Licensed under BSD 3 clause license. See LICENSE.txt file at
 * https://github.com/tudasc/metacg/LICENSE.txt
 */

#include "Callgraph.h"

using namespace metacg;

CgNodePtr Callgraph::getMain() {
  //there is no condition needed
  //if a main node exists it was detected on insertion already, and we return it
  //if a main node doesn't exist, the default is nullptr, so return that
  //revisit this if node deletion is ever supported
  return mainNode;
}

void Callgraph::addEdge(CgNodePtr parentNode, CgNodePtr childNode) {
  parentNode->addChildNode(childNode);
  childNode->addParentNode(parentNode);
}

void Callgraph::addEdge(const std::string &parentName, const std::string &childName) {
  auto pNode = getOrInsertNode(parentName);
  auto cNode = getOrInsertNode(childName);
  assert(pNode != nullptr && "Parent node should be in the graph");
  assert(cNode != nullptr && "Child node should be in the graph");
  addEdge(pNode, cNode);
}

void Callgraph::insert(CgNodePtr node) {
  const auto n = node->getFunctionName();
  // We may want to use string::find() instead of the string compare operator.
  if (n == "main" || n == "_Z4main" || n == "_ZSt4mainiPPc") {
    mainNode = node;
  }
  graph.insert(node);
}

CgNodePtr Callgraph::getOrInsertNode(const std::string &name) {
  auto nodePtr = getNode(name);
  if (nodePtr != nullptr) {
    return nodePtr;
  }
  nodePtr = std::make_shared<CgNode>(name);
  assert(nodePtr != nullptr && "Creating a new CgNode should work");
  insert(nodePtr);
  return nodePtr;
}

void Callgraph::clear() {
  graph.clear();
  lastSearched = nullptr;
  mainNode = nullptr;
}


bool Callgraph::hasNode(std::string name) {
  if (mainNode && name == "main") {
    lastSearched = mainNode;
    return true;
  }

  auto found = std::find_if(graph.begin(), graph.end(), [&](CgNodePtr n) { return n->getFunctionName() == name; });

  if (found != graph.end()) {
    lastSearched = *found;
    return true;
  }

  lastSearched = nullptr;
  return false;
}

bool Callgraph::hasNode(CgNodePtr n) {
  return hasNode(n->getFunctionName());
}

CgNodePtr Callgraph::getLastSearchedNode() { return lastSearched; }

CgNodePtr Callgraph::getNode(std::string name) {
  if(lastSearched && lastSearched->getFunctionName()==name)
    return lastSearched;

  auto found = std::find_if(graph.begin(), graph.end(), [&](CgNodePtr n) { return n->getFunctionName() == name; });
  if (found != graph.end()) {
    return *found;
  }
  return nullptr;
}
size_t Callgraph::size() const { return graph.size(); }

bool Callgraph::isEmpty() { return graph.size() == 0; }
Callgraph::ContainerT &Callgraph::getGraph() { return graph; }

Callgraph::ContainerT::iterator Callgraph::begin() const { return graph.begin(); }
Callgraph::ContainerT::iterator Callgraph::end() const { return graph.end(); }

Callgraph::ContainerT::const_iterator Callgraph::cbegin() const { return graph.cbegin(); }
Callgraph::ContainerT::const_iterator Callgraph::cend() const { return graph.cend(); }

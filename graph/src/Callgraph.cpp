/**
 * File: Callgraph.cpp
 * License: Part of the MetaCG project. Licensed under BSD 3 clause license. See LICENSE.txt file at
 * https://github.com/tudasc/metacg/LICENSE.txt
 */

#include "Callgraph.h"

using namespace metacg;

CgNodePtr Callgraph::findMain() {
  if (mainNode) {
    return mainNode;
  } else {
    // simply search a method containing "main" somewhere
    for (auto node : *this) {
      auto fName = node->getFunctionName();
      if (fName.find("MAIN_") != fName.npos) {
        return node;
      }
    }
    return nullptr;
  }
}

CgNodePtr Callgraph::findNode(std::string functionName) const {
  for (auto node : *this) {
    auto fName = node->getFunctionName();
    if (fName == functionName) {
      return node;
    }
  }
  return nullptr;
}

void Callgraph::insert(CgNodePtr node) {
  const auto n = node->getFunctionName();
  // We may want to use string::find() instead of the string compare operator.
  if (n == "main" || n == "_Z4main" || n == "_ZSt4mainiPPc") {
    mainNode = node;
  }
  graph.insert(node);
}

Callgraph::ContainerT::const_iterator Callgraph::cbegin() const { return graph.cbegin(); }
Callgraph::ContainerT::const_iterator Callgraph::cend() const { return graph.cend(); }

size_t Callgraph::size() const { return graph.size(); }

Callgraph::ContainerT &Callgraph::getGraph() { return graph; }
bool Callgraph::hasNode(std::string name) {
  {
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
}
CgNodePtr Callgraph::getNode(std::string name) {
  {
    auto found = std::find_if(graph.begin(), graph.end(), [&](CgNodePtr n) { return n->getFunctionName() == name; });
    if (found != graph.end()) {
      return *found;
    }
    return nullptr;
  }
}

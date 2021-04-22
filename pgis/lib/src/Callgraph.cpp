/**
 * File: Callgraph.cpp
 * License: Part of the MetaCG project. Licensed under BSD 3 clause license. See LICENSE.txt file at https://github.com/tudasc/metacg/LICENSE.txt
 */

#include "Callgraph.h"

#include <chrono>  // easy time measurement

#define VERBOSE 0
#define DEBUG 0

#define BENCHMARK_PHASES 0
#define PRINT_FINAL_DOT 1
#define PRINT_DOT_AFTER_EVERY_PHASE 1

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

CgNodePtr Callgraph::findNode(std::string functionName) {
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

void Callgraph::eraseInstrumentedNode(CgNodePtr node) {
  // TODO implement
  // since the node is instrumented, all paths till the next conjunction are
  // also instrumented but only if there is exactly one unique path from the
  // instrumented note to the conjunction
}

void Callgraph::erase(CgNodePtr node, bool rewireAfterDeletion, bool force) {
  if (!force) {
    if (CgHelper::isConjunction(node) && node->getChildNodes().size() > 1) {
      std::cerr << "Error: Cannot remove node with multiple parents AND multiple children." << std::endl;
      exit(EXIT_FAILURE);
    }
    if (CgHelper::isConjunction(node) && node->isLeafNode()) {
      std::cerr << "Error: Cannot remove conjunction node that is a leaf." << std::endl;
      exit(EXIT_FAILURE);
    }
  }

  for (auto parent : node->getParentNodes()) {
    parent->removeChildNode(node);
  }
  for (auto child : node->getChildNodes()) {
    child->removeParentNode(node);
  }

  // a conjunction can only be erased if it has exactly one child
  if (!force && CgHelper::isConjunction(node)) {
    auto child = node->getUniqueChild();
    child->getMarkerPositions() = node->getMarkerPositions();

    for (auto markerPosition : node->getMarkerPositions()) {
      markerPosition->getDependentConjunctions().erase(node);
      markerPosition->getDependentConjunctions().insert(child);
    }
    // XXX erasing a node with multiple parents invalidates edge weights (they
    // are visible in the dot output)
  }
  for (auto dependentConjunction : node->getDependentConjunctions()) {
    dependentConjunction->getMarkerPositions().erase(node);
  }

  if (rewireAfterDeletion) {
    for (auto parent : node->getParentNodes()) {
      for (auto child : node->getChildNodes()) {
        parent->addChildNode(child);
        child->addParentNode(parent);
      }
    }
  }

  graph.erase(node);

  // std::cout << "  Erasing node: " << *node << std::endl;
  //	std::cout << "  UseCount: " << node.use_count() << std::endl;
  if (node.use_count() == 0 || node.use_count() == 1 || node.use_count() == 2) {
    std::cout << node->getFunctionName() << " : use count is: " << node.use_count() << std::endl;
  }
}

Callgraph::ContainerT::const_iterator Callgraph::cbegin() const { return graph.cbegin(); }
Callgraph::ContainerT::const_iterator Callgraph::cend() const { return graph.cend(); }

size_t Callgraph::size() const { return graph.size(); }

Callgraph::ContainerT &Callgraph::getGraph() { return graph; }

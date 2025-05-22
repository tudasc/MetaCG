/**
 * File: util.h
 * License: Part of the MetaCG project. Licensed under BSD 3 clause license. See LICENSE.txt file at
 * https://github.com/tudasc/metacg/LICENSE.txt
 */

#pragma once

#include <Callgraph.h>
#include <CgNode.h>
#include <CgNodePtr.h>
#include <MCGManager.h>

/**
 * Helper type that wraps around CgNode* and keeps additional information about which call graph
 * the node is part of.
 * In the graphlib, call graph nodes do not contain a pointer/reference to the call graph that they
 * are a part of. Hence, one must always call a function of the call graph to get a node's callers/callees.
 * To allow for more user-friendly access to callers/callees via Python, pymetacg uses this wrapper to
 * attach the graph reference to each CgNode pointer.
 */
struct CgNodeWrapper {
  const metacg::CgNode* node;
  const metacg::Callgraph& graph;
};

/**
 * Auxiliary iterator to iterate over metacg::Callgraph::NodeContainer and pack CgNode* into CgNodeWrapper
 */
template <typename NodeIterator>
class NodeContainerIteratorWrapper {
 public:
  explicit NodeContainerIteratorWrapper(NodeIterator it, const metacg::Callgraph& graph) : iter(it), graph(graph) {}

  CgNodeWrapper operator*() const { return CgNodeWrapper{iter->second.get(), graph}; }

  NodeContainerIteratorWrapper& operator++() {
    ++iter;
    return *this;
  }

  bool operator==(const NodeContainerIteratorWrapper& other) const { return iter == other.iter; }
  bool operator!=(const NodeContainerIteratorWrapper& other) const { return !(*this == other); }

 private:
  NodeIterator iter;
  const metacg::Callgraph& graph;
};

/**
 * Auxiliary iterator to iterate over metacg::CgNodeRawPtrUSet and pack CgNode* into CgNodeWrapper
 */
template <typename NodeIterator>
class CgNodeRawPtrUSetIteratorWrapper {
 public:
  explicit CgNodeRawPtrUSetIteratorWrapper(NodeIterator it, const metacg::Callgraph& graph) : iter(it), graph(graph) {}

  CgNodeWrapper operator*() const { return CgNodeWrapper{*iter, graph}; }
  CgNodeWrapper operator->() const { return CgNodeWrapper{&iter, graph}; }

  CgNodeRawPtrUSetIteratorWrapper& operator++() {
    ++iter;
    return *this;
  }

  bool operator==(const CgNodeRawPtrUSetIteratorWrapper& other) const { return iter == other.iter; }
  bool operator!=(const CgNodeRawPtrUSetIteratorWrapper& other) const { return !(*this == other); }

 private:
  NodeIterator iter;
  const metacg::Callgraph& graph;
};

/**
 * Helper function to attach the call graph reference to all CgNodes in a CgNodeRawPtrUSet
 * and return a std::unordered_set<CgNodeWrapper, ...>
 */
inline auto attachGraphPointerToNodes(const CgNodeRawPtrUSet nodes, const metacg::Callgraph& graph) {
  constexpr auto hash = [](const CgNodeWrapper& p) { return p.node->getId(); };
  constexpr auto equal = [](const CgNodeWrapper& p1, const CgNodeWrapper& p2) {
    return p1.node->getId() == p2.node->getId();
  };
  std::unordered_set<CgNodeWrapper, decltype(hash), decltype(equal)> newSet(
      CgNodeRawPtrUSetIteratorWrapper(nodes.begin(), graph), CgNodeRawPtrUSetIteratorWrapper(nodes.end(), graph),
      nodes.size(), hash, equal);
  return newSet;
}

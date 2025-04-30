#pragma once

#include <Callgraph.h>
#include <CgNode.h>
#include <CgNodePtr.h>
#include <MCGManager.h>

using namespace metacg;

struct CgNodeWrapper {
  const CgNode* node;
  const Callgraph& graph;
};

template <typename NodeIterator>
class UniquePtrValueIterator {
 public:
  explicit UniquePtrValueIterator(NodeIterator it, const Callgraph& graph) : iter(it), graph(graph) {}

  CgNodeWrapper operator*() const { return CgNodeWrapper{iter->second.get(), graph}; }

  UniquePtrValueIterator& operator++() {
    ++iter;
    return *this;
  }

  bool operator==(const UniquePtrValueIterator& other) const { return iter == other.iter; }
  bool operator!=(const UniquePtrValueIterator& other) const { return !(*this == other); }

 private:
  NodeIterator iter;
  const Callgraph& graph;
};

template <typename NodeIterator>
class WrapperIterator {
 public:
  explicit WrapperIterator(NodeIterator it, const Callgraph& graph) : iter(it), graph(graph) {}

  CgNodeWrapper operator*() const { return CgNodeWrapper{*iter, graph}; }
  CgNodeWrapper operator->() const { return CgNodeWrapper{&iter, graph}; }

  WrapperIterator& operator++() {
    ++iter;
    return *this;
  }

  bool operator==(const WrapperIterator& other) const { return iter == other.iter; }
  bool operator!=(const WrapperIterator& other) const { return !(*this == other); }

 private:
  NodeIterator iter;
  const Callgraph& graph;
};

inline auto attachGraphPointerToNodes(const CgNodeRawPtrUSet nodes, const Callgraph& graph) {
  auto hash = [](const CgNodeWrapper& p) { return p.node->getId(); };
  auto equal = [](const CgNodeWrapper& p1, const CgNodeWrapper& p2) { return p1.node->getId() == p2.node->getId(); };
  std::unordered_set<CgNodeWrapper, decltype(hash), decltype(equal)> newSet(
      WrapperIterator(nodes.begin(), graph), WrapperIterator(nodes.end(), graph), nodes.size(), hash, equal);
  return newSet;
}
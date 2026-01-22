#ifndef METACG_DOMINATORANALYSIS_H
#define METACG_DOMINATORANALYSIS_H

#include <algorithm>
#include <deque>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace metacg::analysis {

enum class TraverseDir { Forward, Backward };

template <typename NodeT>
struct DomData {
  using NodeSet = std::unordered_set<const NodeT*>;
  const NodeT* node{nullptr};
  NodeSet Doms{};
  bool initialized{false};
};

template <typename NodeT>
using DomAnalysisResult = std::unordered_map<const NodeT*, DomData<NodeT>>;

template <typename NodeT, typename GraphT>
DomAnalysisResult<NodeT> computeDoms(const GraphT& graph, const NodeT& exitNode);

template <typename Container>
Container unordered_intersection(const Container& a, const Container& b) {
  //  std::set<typename Container::value_type> orderedA(a.begin(), a.end());
  //  std::set<typename Container::value_type> orderedB(b.begin(), b.end());
  Container result;
  //  std::set_intersection(orderedA.begin(), orderedA.end(),
  //                        orderedB.begin(), orderedB.end(),
  //                        std::inserter(result, result.begin()));
  for (auto& item : a) {
    if (std::find(b.begin(), b.end(), item) != b.end()) {
      result.insert(item);
    }
  }
  return result;
}

// Adopted from https://stackoverflow.com/questions/25505868/the-intersection-of-multiple-sorted-arrays
template <typename NodeT>
typename DomData<NodeT>::NodeSet intersection(const std::vector<const NodeT*>& nodes,
                                              const DomAnalysisResult<NodeT>& DomMap) {
  if (nodes.empty())
    return {};

  auto last_intersection = DomMap.at(nodes[0]).Doms;

  typename DomData<NodeT>::NodeSet curr_intersection;

  for (std::size_t i = 1; i < nodes.size(); ++i) {
    auto& currSet = DomMap.at(nodes[i]).Doms;
    // LOG_STATUS("Intersecting " << dumpNodeSet("a", last_intersection) << " and " << dumpNodeSet("b", currSet) <<
    // "\n");

    // Note: std::set_intersection does not work with unordered_set.
    curr_intersection = unordered_intersection(last_intersection, currSet);
    //    std::set_intersection(last_intersection.begin(), last_intersection.end(),
    //                          currSet.begin(), currSet.end(),
    //                          std::inserter(curr_intersection, curr_intersection.begin()));
    // LOG_STATUS("Result of intersection" << dumpNodeSet("", curr_intersection) << "\n");
    std::swap(last_intersection, curr_intersection);
    curr_intersection.clear();
  }
  return last_intersection;
}

template <typename NodeT, typename GraphT, TraverseDir dir>
DomAnalysisResult<NodeT> computeDoms(const GraphT& graph, const NodeT& exitNode) {
  auto incoming = [&](const NodeT* n) {
    if constexpr (dir == TraverseDir::Forward)
      return graph.getCallers(*n);
    else
      return graph.getCallees(*n);
  };

  auto outgoing = [&](const NodeT* n) {
    if constexpr (dir == TraverseDir::Backward)
      return graph.getCallers(*n);
    else
      return graph.getCallees(*n);
  };

  using DomDataT = DomData<NodeT>;

  DomAnalysisResult<NodeT> DomMap{{&exitNode, {&exitNode, {}, false}}};

  std::deque<DomDataT*> workQueue{&DomMap[&exitNode]};

  auto addToQueue = [&workQueue](DomDataT* const data) {
    if (std::find(workQueue.begin(), workQueue.end(), data) == workQueue.end()) {
      workQueue.push_back(data);
    }
  };

  do {
    auto& nodeData = *workQueue.front();
    workQueue.pop_front();

    std::vector<const NodeT*> initializedCallees{};
    for (auto* calleePtr : incoming(nodeData.node)) {
      if (DomMap[calleePtr].initialized) {
        initializedCallees.push_back(calleePtr);
      }
    }

    typename DomDataT::NodeSet DomNew = intersection(initializedCallees, DomMap);
    DomNew.insert(nodeData.node);

    // LOG_STATUS("New Doms " << dumpNodeSet(nodeData.node->getName(), DomNew) << "\n");

    if (!nodeData.initialized || nodeData.Doms != DomNew) {
      nodeData.Doms = std::move(DomNew);
      nodeData.initialized = true;

      auto outs = outgoing(nodeData.node);
      for (auto* callerPtr : outs) {
        auto& data = DomMap[callerPtr];
        if (!data.node) {
          data.node = callerPtr;
        }
        addToQueue(&data);
      }
    }
  } while (!workQueue.empty());

  return DomMap;
}

}  // namespace metacg::analysis

#endif

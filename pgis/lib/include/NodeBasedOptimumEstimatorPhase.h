
#ifndef NODEBASEDOPTIMUMESTIMATORPHASE_H_
#define NODEBASEDOPTIMUMESTIMATORPHASE_H_

#include "CgHelper.h"
#include "CgNode.h"
#include "EstimatorPhase.h"

#include <functional>  // std::hash
#include <stack>
#include <vector>

struct NodeBasedConstraint;
struct NodeBasedState;

typedef std::vector<NodeBasedConstraint> NodeBasedConstraintContainer;

class OptimalNodeBasedEstimatorPhase : public EstimatorPhase {
 public:
  OptimalNodeBasedEstimatorPhase();
  ~OptimalNodeBasedEstimatorPhase();

  void modifyGraph(CgNodePtr mainMethod);

 protected:
  void printAdditionalReport();

 private:
  std::stack<NodeBasedState> stateStack;

  CgNodePtrSet optimalInstrumentation;
  unsigned long long optimalCosts;

  unsigned long long numberOfStepsTaken;
  unsigned long long numberOfStepsAvoided;

  // optimization for large data sets
  std::set<std::size_t> visitedCombinations;

  void findStartingState(CgNodePtr mainMethod);

  void step(NodeBasedState &startState);
};

struct NodeBasedConstraint {
  CgNodePtrSet elements;
  CgNodePtr conjunctionNode;  // maybe this will come handy later

  NodeBasedConstraint(CgNodePtrSet elements, CgNodePtr conjunction) {
    this->elements = elements;
    this->conjunctionNode = conjunction;
  }

  inline bool validAfterExchange(CgNodePtr oldElement, CgNodePtrSet newElements) {
    if (elements.find(oldElement) != elements.end()) {
      CgNodePtrSet intersection = CgHelper::setIntersect(elements, newElements);
      elements.insert(newElements.begin(), newElements.end());

      return intersection.empty();
    } else {
      return true;
    }
  }

  friend std::ostream &operator<<(std::ostream &stream, const NodeBasedConstraint &c) {
    stream << "[";
    for (auto element : c.elements) {
      stream << *element << ", ";
    }
    stream << "]";

    return stream;
  }
};

struct NodeBasedState {
  CgNodePtrSet nodeSet;
  NodeBasedConstraintContainer constraints;

  NodeBasedState(CgNodePtrSet nodeSet, NodeBasedConstraintContainer constraints) {
    this->nodeSet = nodeSet;
    this->constraints = constraints;
  }

  inline bool validAfterExchange(CgNodePtr oldElement, CgNodePtrSet newElements) {
    if (nodeSet.find(oldElement) != nodeSet.end()) {
      nodeSet.erase(oldElement);
      nodeSet.insert(newElements.begin(), newElements.end());
    }
    for (auto &constraint : constraints) {
      if (!constraint.validAfterExchange(oldElement, newElements)) {
        return false;
      }
    }

    return true;
  }

  inline unsigned long long getCosts() {
    // note that the scumbag zero will break everything unless it is explicitly "ULL"
    return std::accumulate(nodeSet.begin(), nodeSet.end(), 0ULL,
                           [](unsigned long long calls, CgNodePtr node) { return calls + node->getNumberOfCalls(); });
  }

  friend std::ostream &operator<<(std::ostream &stream, const NodeBasedState &state) {
    stream << "-- marked: ";
    for (auto node : state.nodeSet) {
      stream << *node << ", ";
    }
    stream << "-- constraints: ";
    for (auto c : state.constraints) {
      stream << c << ", ";
    }
    stream << "--";

    return stream;
  }
};

template <class T>
inline std::size_t hashCombine(const std::size_t seed, const T toBeHashed) {
  // according to stackoverflow this is a decent hash function
  return seed ^ (std::hash<T>()(toBeHashed) + 0x9e3779b9UL + (seed << 6) + (seed >> 2));
}

namespace std {

template <>
struct hash<CgNodePtrSet> {
  inline size_t operator()(const CgNodePtrSet &key) const {
    return std::accumulate(key.begin(), key.end(), (size_t)0, [](size_t acc, const CgNodePtr n) {
      // use pointer address for hash of CgNode
      return hashCombine<size_t>(acc, (size_t)n.get());
    });
  }
};

template <>
struct hash<NodeBasedConstraintContainer> {
  size_t operator()(const NodeBasedConstraintContainer &key) const {
    return std::accumulate(key.begin(), key.end(), (size_t)0, [](size_t acc, const NodeBasedConstraint c) {
      return hashCombine<CgNodePtrSet>(acc, c.elements);
    });
  }
};

template <>
struct hash<NodeBasedState> {
  size_t operator()(const NodeBasedState &key) const {
    size_t nodeSetHash = hash<CgNodePtrSet>()(key.nodeSet);
    return hashCombine<NodeBasedConstraintContainer>(nodeSetHash, key.constraints);
  }
};
}  // namespace std

#endif

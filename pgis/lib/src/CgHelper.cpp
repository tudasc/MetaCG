#include "CgHelper.h"
#include "Callgraph.h"

int CgConfig::samplesPerSecond = 10000;

namespace CgHelper {

/** returns true for nodes with two or more parents */
bool isConjunction(CgNodePtr node) { return (node->getParentNodes().size() > 1); }

/** returns overhead of all the instrumented nodes of the call path. (node
 * based) */
unsigned long long getInstrumentationOverheadOfPath(CgNodePtr node) {
  unsigned long long costInNanos = 0ULL;

  CgNodePtrSet instrumentationPaths = getInstrumentationPath(node);
  for (auto potentiallyMarked : instrumentationPaths) {
    // RN: the main function can be instrumented as this is part of the
    // heuristic
    if (potentiallyMarked->isInstrumentedWitness()) {
      costInNanos += potentiallyMarked->getNumberOfCalls() * CgConfig::nanosPerInstrumentedCall;
    }
  }

  return costInNanos;
}

/** returns a pointer to the node that is instrumented up that call path */
// TODO: check because of new nodeBased Conventions
CgNodePtr getInstrumentedNodeOnPath(CgNodePtr node) {
  // XXX RN: this method has slowly grown up to a real mess
  if (node->isInstrumentedWitness() || node->isRootNode()) {
    return node;
  }

  if (isConjunction(node)) {
    return nullptr;
  }
  // single parent
  auto parentNode = node->getUniqueParent();

  return getInstrumentedNodeOnPath(parentNode);
}

/** Returns a set of all nodes from the starting node up to the instrumented
 * nodes.
 *  It should not break for cycles, because cycles have to be instrumented by
 * definition. */
CgNodePtrSet getInstrumentationPath(CgNodePtr start) {
  CgNodePtrUnorderedSet path;  // visited nodes
  std::queue<CgNodePtr> workQueue;
  workQueue.push(start);

  while (!workQueue.empty()) {
    auto node = workQueue.front();
    workQueue.pop();

    path.insert(node);

    if (node->isInstrumented() || node->isRootNode()) {
      continue;
    }

    for (auto parentNode : node->getParentNodes()) {
      if (path.find(parentNode) == path.end()) {
        workQueue.push(parentNode);
      }
    }
  }

  return CgNodePtrSet(path.begin(), path.end());
}

bool deleteInstrumentationIfRedundant(CgNodePtr instrumentedNode) {
  auto childNodes = instrumentedNode->getChildNodes();
  if (childNodes.find(instrumentedNode) != childNodes.end()) {
    return false;
  }

  for (auto childOfParentNode : childNodes) {
    if (CgHelper::isConjunction(childOfParentNode) &&
        !(childOfParentNode->isUnwound() || childOfParentNode->isInstrumentedConjunction())) {
      return false;
    }
  }

  if (instrumentedNode->isInstrumented()) {
    instrumentedNode->setState(CgNodeState::NONE);
  }
  return true;
}

bool isUniquelyInstrumented(CgNodePtr conjunctionNode, CgNodePtr unInstrumentedNode, bool printErrors) {
  if ((conjunctionNode->isInstrumentedConjunction() && conjunctionNode != unInstrumentedNode) ||
      conjunctionNode->isUnwound()) {
    return true;
  }

  CgNodePtrSet visited;  // visited nodes

  std::queue<CgNodePtr> workQueue;
  for (auto parentNode : conjunctionNode->getParentNodes()) {
    workQueue.push(parentNode);
  }

  while (!workQueue.empty()) {
    auto node = workQueue.front();
    workQueue.pop();

    if (visited.find(node) == visited.end()) {
      visited.insert(node);
    } else {
      if (printErrors) {
        std::cerr << "Error: the conjunction: " << *conjunctionNode << " is reached on multiple paths by: " << *node
                  << std::endl;
      }
      return false;
    }

    if ((node != unInstrumentedNode && node->isInstrumented()) || node->isRootNode()) {
      continue;
    }

    for (auto parentNode : node->getParentNodes()) {
      workQueue.push(parentNode);
    }
  }
  return true;
}

/**
 * Checks the instrumentation paths (node based!) above a conjunction node for
 * intersection. Returns the Number Of Errors ! */
int uniquelyInstrumentedConjunctionTest(CgNodePtr conjunctionNode, bool printErrors) {
  int numberOfErrors = 0;

  auto parents = conjunctionNode->getParentNodes();
  std::map<CgNodePtr, CgNodePtrSet> paths;

  for (auto parentNode : parents) {
    CgNodePtrSet path = getInstrumentationPath(parentNode);
    paths[parentNode] = path;
  }

  for (auto pair : paths) {
    for (auto otherPair : paths) {
      if (pair == otherPair) {
        continue;
      }

      if (intersects(pair.second, otherPair.second)) {
        if (printErrors) {
          std::cout << "ERROR in conjunction: " << *conjunctionNode << std::endl;
          std::cout << "    "
                    << "Paths of " << *(pair.first) << " and " << *(otherPair.first) << " intersect!" << std::endl;
        }
        numberOfErrors++;
      }
    }
  }

  return numberOfErrors;
}

/** returns the overhead caused by a call path */
// TODO this will only work with direct parents instrumented
unsigned long long getInstrumentationOverheadOfConjunction(CgNodePtr conjunctionNode) {
  auto parents = conjunctionNode->getParentNodes();

  CgNodePtrSet potentiallyInstrumented;
  for (auto parentNode : parents) {
    auto tmpSet = getInstrumentationPath(parentNode);
    potentiallyInstrumented.insert(tmpSet.begin(), tmpSet.end());
  }

  // add costs if node is instrumented
  return std::accumulate(potentiallyInstrumented.begin(), potentiallyInstrumented.end(), 0ULL,
                         [](unsigned long long acc, CgNodePtr node) {
                           if (node->isInstrumentedWitness()) {
                             return acc + (node->getNumberOfCalls() * CgConfig::nanosPerInstrumentedCall);
                           }
                           return acc;
                         });
}

/** returns the overhead caused by a call path */
// TODO this will only work with direct parents instrumented
unsigned long long getInstrumentationOverheadOfConjunction(CgNodePtrSet conjunctionNodes) {
  CgNodePtrSet allParents;
  for (auto n : conjunctionNodes) {
    CgNodePtrSet parents = n->getParentNodes();
    allParents.insert(parents.begin(), parents.end());
  }

  CgNodePtrSet potentiallyInstrumented;

  for (auto parentNode : allParents) {
    auto tmpSet = getInstrumentationPath(parentNode);
    potentiallyInstrumented.insert(tmpSet.begin(), tmpSet.end());
  }

  // add costs if node is instrumented
  return std::accumulate(potentiallyInstrumented.begin(), potentiallyInstrumented.end(), 0ULL,
                         [](unsigned long long acc, CgNodePtr node) {
                           if (node->isInstrumentedWitness()) {
                             return acc + (node->getNumberOfCalls() * CgConfig::nanosPerInstrumentedCall);
                           }
                           return acc;
                         });
}

unsigned long long getInstrumentationOverheadServingOnlyThisConjunction(CgNodePtr conjunctionNode) {
  auto parents = conjunctionNode->getParentNodes();

  CgNodePtrSet potentiallyInstrumented;
  for (auto parentNode : parents) {
    auto tmpSet = getInstrumentationPath(parentNode);
    potentiallyInstrumented.insert(tmpSet.begin(), tmpSet.end());
  }

  // add costs if node is instrumented
  return std::accumulate(potentiallyInstrumented.begin(), potentiallyInstrumented.end(), 0ULL,
                         [](unsigned long long acc, CgNodePtr node) {
                           bool onlyOneDependendConjunction = node->getDependentConjunctionsConst().size() == 1;
                           if (node->isInstrumentedWitness() && onlyOneDependendConjunction) {
                             return acc + (node->getNumberOfCalls() * CgConfig::nanosPerInstrumentedCall);
                           }
                           return acc;
                         });
}

unsigned long long getInstrumentationOverheadServingOnlyThisConjunction(CgNodePtrSet conjunctionNodes) {
  CgNodePtrSet allParents;
  for (auto n : conjunctionNodes) {
    CgNodePtrSet parents = n->getParentNodes();
    allParents.insert(parents.begin(), parents.end());
  }

  CgNodePtrSet potentiallyInstrumented;
  for (auto parentNode : allParents) {
    auto tmpSet = getInstrumentationPath(parentNode);
    potentiallyInstrumented.insert(tmpSet.begin(), tmpSet.end());
  }

  // add costs if node is instrumented
  return std::accumulate(potentiallyInstrumented.begin(), potentiallyInstrumented.end(), 0ULL,
                         [](unsigned long long acc, CgNodePtr node) {
                           bool onlyOneDependendConjunction = true;
                           for (auto dependentConj : node->getDependentConjunctionsConst()) {
                             if (dependentConj != node && dependentConj->isUnwound()) {
                               onlyOneDependendConjunction = false;
                             }
                           }

                           if (node->isInstrumentedWitness() && onlyOneDependendConjunction) {
                             return acc + (node->getNumberOfCalls() * CgConfig::nanosPerInstrumentedCall);
                           }
                           return acc;
                         });
}

/** removes the instrumentation of a call path.
 * 	returns false if no instrumentation found */
// TODO: check because of new nodeBased Conventions
bool removeInstrumentationOnPath(CgNodePtr node) {
  if (node->isInstrumentedWitness()) {
    node->setState(CgNodeState::NONE);
    return true;
  }
  if (isConjunction(node) || node->isRootNode()) {
    return false;
  }

  // there can not be instrumentation up here if the parent has multiple
  // children
  auto uniqueParent = node->getUniqueParent();
  if (uniqueParent->getChildNodes().size() > 1) {
    return false;
  }

  return removeInstrumentationOnPath(node->getUniqueParent());
}

// Graph Stats
CgNodePtrSet getPotentialMarkerPositions(CgNodePtr conjunction) {
  CgNodePtrSet potentialMarkerPositions;

  if (!CgHelper::isConjunction(conjunction)) {
    return potentialMarkerPositions;
  }

  CgNodePtrSet visitedNodes;
  std::queue<CgNodePtr> workQueue;
  workQueue.push(conjunction);

  while (!workQueue.empty()) {
    auto node = workQueue.front();
    workQueue.pop();

    for (auto &parentNode : node->getParentNodes()) {
      if (visitedNodes.find(parentNode) != visitedNodes.end()) {
        continue;
      } else {
        visitedNodes.insert(parentNode);
      }

      if (isValidMarkerPosition(parentNode, conjunction)) {
        potentialMarkerPositions.insert(parentNode);
        workQueue.push(parentNode);
      }
    }
  }

  assert(potentialMarkerPositions.size() >= (conjunction->getParentNodes().size() - 1));

  return potentialMarkerPositions;
}

bool isValidMarkerPosition(CgNodePtr markerPosition, CgNodePtr conjunction) {
  if (isOnCycle(markerPosition)) {
    return true;  // nodes on cycles are always valid marker positions
  }

  // if one parent of the conjunction parents is unreachable -> valid marker
  for (auto parentNode : conjunction->getParentNodes()) {
    if (!reachableFrom(markerPosition, parentNode)) {
      return true;
    }
  }
  return false;
}

bool isOnCycle(CgNodePtr node) {
  CgNodePtrSet visitedNodes;
  std::queue<CgNodePtr> workQueue;
  workQueue.push(node);
  while (!workQueue.empty()) {
    auto currentNode = workQueue.front();
    workQueue.pop();

    if (visitedNodes.count(currentNode) == 0) {
      visitedNodes.insert(currentNode);

      for (auto child : currentNode->getChildNodes()) {
        if (child == node) {
          return true;
        }
        workQueue.push(child);
      }
    }
  }
  return false;
}

CgNodePtrSet getReachableConjunctions(CgNodePtrSet markerPositions) {
  CgNodePtrSet reachableConjunctions;

  CgNodePtrSet visitedNodes;
  std::queue<CgNodePtr> workQueue;
  for (auto markerPos : markerPositions) {
    workQueue.push(markerPos);
  }

  while (!workQueue.empty()) {
    auto node = workQueue.front();
    workQueue.pop();

    for (auto child : node->getChildNodes()) {
      if (visitedNodes.find(child) != visitedNodes.end()) {
        continue;
      } else {
        visitedNodes.insert(child);
      }

      workQueue.push(child);

      if (CgHelper::isConjunction(child)) {
        reachableConjunctions.insert(child);
      }
    }
  }
  return reachableConjunctions;
}

// note: a function is reachable from itself
bool reachableFrom(CgNodePtr parentNode, CgNodePtr childNode) {
  if (parentNode == childNode) {
    return true;
  }

  // XXX RN: once again code duplication
  CgNodePtrSet visitedNodes;
  std::queue<CgNodePtr> workQueue;
  workQueue.push(parentNode);

  while (!workQueue.empty()) {
    auto node = workQueue.front();
    workQueue.pop();

    if (node == childNode) {
      return true;
    }

    visitedNodes.insert(node);

    for (auto childNode : node->getChildNodes()) {
      if (visitedNodes.find(childNode) == visitedNodes.end()) {
        workQueue.push(childNode);
      }
    }
  }

  return false;
}

CgNodePtrUnorderedSet allNodesToMain(CgNodePtr startNode, CgNodePtr mainNode, const std::unordered_map<CgNodePtr, CgNodePtrUnorderedSet> &init) {
  {
  auto it = init.find(startNode);
  if (it != init.end()) {
    return (*it).second;
  }
  }

  CgNodePtrUnorderedSet pNodes;
  pNodes.insert(mainNode);

  CgNodePtrUnorderedSet visitedNodes;
  std::queue<CgNodePtr> workQueue;
  workQueue.push(startNode);

  while (!workQueue.empty()) {
    auto node = workQueue.front();
    workQueue.pop();

    visitedNodes.insert(node);

    // XXX This is itself an expensive computation!
  //  if (reachableFrom(mainNode, node)){
  //    pNodes.insert(node);
  //  }
    if (!node->isReachable()) {
      continue;
    } else {
      pNodes.insert(node);
    }

    auto pns = node->getParentNodes();
    for (auto pNode : pns) {
      if (visitedNodes.find(pNode) == visitedNodes.end()) {
        workQueue.push(pNode);
      }
    }
  }

  return pNodes;
}


CgNodePtrUnorderedSet allNodesToMain(CgNodePtr startNode, CgNodePtr mainNode) {
  return allNodesToMain(startNode, mainNode, {});
}


/** true if the two nodes are connected via spanning tree edges */
// XXX RN: this method is ugly and has horrible complexity
// XXX RN: deprecated!
bool isConnectedOnSpantree(CgNodePtr n1, CgNodePtr n2) {
  CgNodePtrSet reachableNodes = {n1};
  size_t size = 0;

  while (size != reachableNodes.size()) {
    size = reachableNodes.size();

    // RN:	note that elements are inserted during iteration
    // 		bad things may happen if an unordered container is used here
    for (auto node : reachableNodes) {
      for (auto parentNode : node->getParentNodes()) {
        if (node->isSpantreeParent(parentNode)) {
          reachableNodes.insert(parentNode);
        }
      }
      for (auto childNode : node->getChildNodes()) {
        if (childNode->isSpantreeParent(node)) {
          reachableNodes.insert(childNode);
        }
      }
    }
  }
  return reachableNodes.find(n2) != reachableNodes.end();
}

/** Returns true if the unique call path property for edges is violated
 *  once this edge is added */
bool canReachSameConjunction(CgNodePtr below, CgNodePtr above) {
  CgNodePtrSet belowReachableDescendants = getDescendants(below);

  CgNodePtrSet aboveReachableDescendants;
  for (auto ancestor : getAncestors(above)) {
    CgNodePtrSet descendants = getDescendants(ancestor);
    aboveReachableDescendants.insert(descendants.begin(), descendants.end());
  }

  CgNodePtrSet intersect = setIntersect(belowReachableDescendants, aboveReachableDescendants);

  return !intersect.empty();
}

/** Returns a set of all descendants including the starting node */
CgNodePtrSet getDescendants(CgNodePtr startingNode) {
  // CgNodePtrUnorderedSet childs;
  CgNodePtrSet childs;
  std::queue<CgNodePtr> workQueue;
  workQueue.push(startingNode);

  while (!workQueue.empty()) {
    auto node = workQueue.front();
    workQueue.pop();

    childs.insert(node);

    for (auto childNode : node->getChildNodes()) {
      if (childs.find(childNode) == childs.end()) {
        workQueue.push(childNode);
      }
    }
  }
  return childs;
}

/** Returns a set of all ancestors including the startingNode */
CgNodePtrSet getAncestors(CgNodePtr startingNode) {
  CgNodePtrSet ancestors;
  std::queue<CgNodePtr> workQueue;
  workQueue.push(startingNode);

  while (!workQueue.empty()) {
    auto node = workQueue.front();
    workQueue.pop();

    ancestors.insert(node);

    for (auto parentNode : node->getParentNodes()) {
      if (ancestors.find(parentNode) == ancestors.end()) {
        workQueue.push(parentNode);
      }
    }
  }

  return ancestors;
}

double calcRuntimeThreshold(const Callgraph &cg, bool useLongAsRef) {
  std::vector<double> rt;
  for (const auto &n : cg) {
    if (n->comesFromCube()) {
      rt.push_back(n->getInclusiveRuntimeInSeconds());
    }
  }
  std::cout << "Basis for runtime threshold calculation: " << rt.size() << "\n";

  std::sort(rt.begin(), rt.end());
  for (const auto r : rt) {
    std::cout << r << " seconds\n";
  }

  size_t lastIndex = rt.size() * .5;
  if (useLongAsRef) {
    lastIndex = rt.size() - 2;  // use the function after main.
    return rt[lastIndex] / 2;   // halve this value (first idea)
  }
  return rt[lastIndex];
}

double calcInclusiveRuntime(CgNode *node) {
  double runTime = 0.0;

  std::queue<const CgNode *> workQueue;
  workQueue.push(node);
  std::unordered_set<const CgNode *> visitedNodes;

  while (!workQueue.empty()) {
    auto node = workQueue.front();
    workQueue.pop();

    visitedNodes.insert(node);

    // Only count the runtime of nodes comming from the profile
    if (node->comesFromCube()) {
      runTime += node->getRuntimeInSeconds();
    }

    for (auto childNode : node->getChildNodes()) {
      // Only visit unseen, profiled nodes. Only those have actual timing info!
      CgNode *cn = childNode.get();
      if (visitedNodes.find(cn) == visitedNodes.end() && childNode->comesFromCube()) {
        workQueue.push(cn);
      }
    }
  }

  // startNode->setInclusiveRuntimeInSeconds(runTime);
  return runTime;
}


}  // namespace CgHelper

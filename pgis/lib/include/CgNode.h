#ifndef CG_NODE_H
#define CG_NODE_H

#include <fstream>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include <map>
#include <queue>
#include <set>

#include "CgLocation.h"
#include "ExtrapConnection.h"
#include <algorithm>
#include <unordered_set>

// iterate priority_queue as of: http://stackoverflow.com/a/1385520
template <class T, class S, class C>
S &Container(std::priority_queue<T, S, C> &q) {
  struct HackedQueue : private std::priority_queue<T, S, C> {
    static S &Container(std::priority_queue<T, S, C> &q) { return q.*&HackedQueue::c; }
  };
  return HackedQueue::Container(q);
}

class CgNode;

namespace std {
template <>
struct less<std::shared_ptr<CgNode>> {
  bool operator()(const std::shared_ptr<CgNode> &a, const std::shared_ptr<CgNode> &b) const;
};
template <>
struct less_equal<std::shared_ptr<CgNode>> {
  bool operator()(const std::shared_ptr<CgNode> &a, const std::shared_ptr<CgNode> &b) const;
};

template <>
struct equal_to<std::shared_ptr<CgNode>> {
  bool operator()(const std::shared_ptr<CgNode> &a, const std::shared_ptr<CgNode> &b) const;
};
template <>
struct greater<std::shared_ptr<CgNode>> {
  bool operator()(const std::shared_ptr<CgNode> &a, const std::shared_ptr<CgNode> &b) const;
};

template <>
struct greater_equal<std::shared_ptr<CgNode>> {
  bool operator()(const std::shared_ptr<CgNode> &a, const std::shared_ptr<CgNode> &b) const;
};
}  // namespace std

enum CgNodeState { NONE, INSTRUMENT_WITNESS, UNWIND_SAMPLE, INSTRUMENT_CONJUNCTION, UNWIND_INSTR, INSTRUMENT_CUBE };

typedef std::shared_ptr<CgNode> CgNodePtr;  // hopefully this typedef helps readability

typedef std::set<CgNodePtr> CgNodePtrSet;
typedef std::unordered_set<CgNodePtr> CgNodePtrUnorderedSet;

class CgNode {
 public:
  CgNode(std::string function);
  CgNode(const CgNode &other) = delete;
  CgNode(const CgNode &&other) = delete;

  CgNode &operator=(CgNode other) = delete;
  CgNode &operator=(const CgNode &other) = delete;
  CgNode &operator=(CgNode &&other) = delete;

  void addChildNode(CgNodePtr childNode);
  void addParentNode(CgNodePtr parentNode);
  void removeChildNode(CgNodePtr childNode);
  void removeParentNode(CgNodePtr parentNode);

  bool isSameFunction(CgNodePtr otherNode) const;

  std::string getFunctionName() const;
  int getLineNumber() const { return line; }

  const CgNodePtrSet &getChildNodes() const;
  const CgNodePtrSet &getParentNodes() const;
  const CgNodePtrSet &getSpantreeParents() const { return spantreeParents; }

  void addCallData(CgNodePtr parentNode, unsigned long long calls, double timeInSeconds, int threadId, int procId);
  unsigned long long getNumberOfCalls() const;
  unsigned long long getNumberOfCallsWithCurrentEdges() const;
  unsigned long long getNumberOfCalls(CgNodePtr parentNode);
  std::vector<CgLocation> getCgLocation() const;

  void setNumberOfStatements(int numStmts);
  int getNumberOfStatements() const;
  void setComesFromCube() { this->wasInPreviousProfile = true; }
  bool comesFromCube() const { return this->wasInPreviousProfile || !this->filename.empty(); }
  bool inPreviousProfile() const { return wasInPreviousProfile; }

  void setExtrapModelConnector(extrapconnection::ExtrapConnector epCon) { this->epCon = epCon; }
  auto &getExtrapModelConnector() { return epCon; }

  void setReachable() { this->reachable = true; }
  bool isReachable() const { return this->reachable; }
  void setDominantRuntime() { this->dominantRuntime = true; }
  bool isDominantRuntime() const { return this->dominantRuntime; }

  double getRuntimeInSeconds() const;
  /* FIXME: The behaviour of this method needs to be easier. */
  double getInclusiveRuntimeInSeconds();
  void setInclusiveRuntimeInSeconds(double newInclusiveRuntimeInSeconds);
  void setRuntimeInSeconds(double newRuntimeInSeconds);
  unsigned long long getExpectedNumberOfSamples() const;

  void setState(CgNodeState state, int numberOfUnwindSteps = 0);
  CgNodeState getStateRaw() const;
  bool isInstrumented() const;
  bool isInstrumentedWitness() const;
  bool isInstrumentedConjunction() const;
  bool isUnwound() const;
  bool isUnwoundSample() const;
  bool isUnwoundInstr() const;

  int getNumberOfUnwindSteps() const;

  // marker pos & dependent conjunction stuff
  CgNodePtrSet &getMarkerPositions();
  CgNodePtrSet &getDependentConjunctions();
  const CgNodePtrSet &getMarkerPositionsConst() const;
  const CgNodePtrSet &getDependentConjunctionsConst() const;

  // spanning tree stuff
  void addSpantreeParent(CgNodePtr parentNode);
  bool isSpantreeParent(CgNodePtr);
  void reset();

  void updateNodeAttributes(bool updateNumberOfSamples = true);
  void updateExpectedNumberOfSamples();
  void setExpectedNumberOfSamples(unsigned long long samples);

  bool hasUniqueCallPath() const;
  bool isLeafNode() const;
  bool isRootNode() const;

  bool hasUniqueParent() const;
  bool hasUniqueChild() const;
  CgNodePtr getUniqueParent() const;
  CgNodePtr getUniqueChild() const;

  void setFilename(std::string filename);
  void setLineNumber(int line);

  void dumpToDot(std::ofstream &outputStream, int procNum);

  void print();
  void printMinimal();

  void setDominance(CgNodePtr child, double dominance);
  double getDominance(CgNodePtr child);

  friend std::ostream &operator<<(std::ostream &stream, const CgNode &n);
  friend std::ostream &operator<<(std::ostream &stream, const CgNodePtrSet &s);

 private:
  std::string functionName;
  CgNodeState state;

  int numberOfUnwindSteps;
  unsigned long long numberOfCalls;
  int numberOfStatements;

  extrapconnection::ExtrapConnector epCon;

  // note that these metrics are based on a profile and might be pessimistic
  double runtimeInSeconds;
  double inclusiveRuntimeInSeconds;
  unsigned long long expectedNumberOfSamples;
  std::vector<CgLocation> cgLoc;

  bool wasInPreviousProfile;

  bool reachable;
  bool dominantRuntime;

  CgNodePtrSet childNodes;
  CgNodePtrSet parentNodes;

  // parentNode -> number of calls by that parent
  std::map<CgNodePtr, unsigned long long> numberOfCallsBy;

  //
  std::map<CgNodePtr, double> dominanceMap;

  // if the node is a conjunction, these are the potentially instrumented nodes
  CgNodePtrSet potentialMarkerPositions;
  // if the node is a potential marker position, these conjunctions depend on its instrumentation
  CgNodePtrSet dependentConjunctions;

  // this is possibly the dumbest way to implement a spanning tree
  CgNodePtrSet spantreeParents;

  // node attributes
  bool uniqueCallPath;

  // for later use
  std::string filename;
  int line;
};

struct CalledMoreOften {
  bool operator()(const CgNodePtr &lhs, const CgNodePtr &rhs) {
    return lhs->getNumberOfCalls() < rhs->getNumberOfCalls();
  }
};
typedef std::priority_queue<CgNodePtr, std::vector<CgNodePtr>, CalledMoreOften> CgNodePtrQueueMostCalls;

struct MoreRuntimeAndLeavesFirst {
  bool operator()(const CgNodePtr &lhs, const CgNodePtr &rhs) {
    if (!lhs->isLeafNode() && rhs->isLeafNode()) {
      return true;
    }
    if (lhs->isLeafNode() && !rhs->isLeafNode()) {
      return false;
    }
    return lhs->getRuntimeInSeconds() < rhs->getRuntimeInSeconds();
  }
};
typedef std::priority_queue<CgNodePtr, std::vector<CgNodePtr>, MoreRuntimeAndLeavesFirst> CgNodePtrQueueUnwHeur;

struct CgEdge {
  CgNodePtr from;
  CgNodePtr to;

  CgEdge(CgNodePtr from, CgNodePtr to) : from(from), to(to) {}

  bool operator<(const CgEdge &other) const { return std::tie(from, to) < std::tie(other.from, other.to); }

  friend bool operator==(const CgEdge &lhs, const CgEdge &rhs) {
    return std::tie(lhs.from, lhs.to) == std::tie(rhs.from, rhs.to);
  }

  friend std::ostream &operator<<(std::ostream &stream, const CgEdge &e) {
    stream << *(e.from) << " -> " << *(e.to);
    return stream;
  }
};

#endif

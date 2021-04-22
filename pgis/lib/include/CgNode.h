/**
 * File: CgNode.h
 * License: Part of the MetaCG project. Licensed under BSD 3 clause license. See LICENSE.txt file at https://github.com/tudasc/metacg/LICENSE.txt
 */

#ifndef CG_NODE_H
#define CG_NODE_H

#include "CgLocation.h"
#include "CgNodeMetaData.h"
#include "CgNodePtr.h"

#include <map>
#include <queue>
#include <string>
#include <vector>

// iterate priority_queue as of: http://stackoverflow.com/a/1385520
template <class T, class S, class C>
S &Container(std::priority_queue<T, S, C> &q) {
  struct HackedQueue : private std::priority_queue<T, S, C> {
    static S &Container(std::priority_queue<T, S, C> &q) { return q.*&HackedQueue::c; }
  };
  return HackedQueue::Container(q);
}

enum CgNodeState {
  NONE,
  INSTRUMENT_WITNESS,
  UNWIND_SAMPLE,
  INSTRUMENT_CONJUNCTION,
  UNWIND_INSTR,
  INSTRUMENT_CUBE,
  INSTRUMENT_PATH
};

class CgNode {
  using MetaData = pira::MetaData;
  // XXX Meta package
  mutable std::unordered_map<std::string, MetaData *> metaFields;

 public:
  template <typename T>
  inline bool has() const {
    return metaFields.find(T::key()) != metaFields.end();
  }

  template <typename T>
  inline T *get() const {
    assert(metaFields[T::key()] && "meta field with value " T::key() " exists");
    auto val = metaFields[T::key()];
    return reinterpret_cast<T *>(val);
  }

  template <typename T>
  inline std::pair<bool, T *> checkAndGet() const {
    spdlog::get("console")->trace("checkAndGet() [{}]", this->getFunctionName());
    if (this->has<T>()) {
      auto bpd = this->get<T>();
      assert(bpd && "Pira data attached");
      return {true, bpd};
    }
    spdlog::get("console")->trace("checkAndGet() return nullptr [{}]", this->getFunctionName());
    return {false, nullptr};
  }

  template <typename T>
  inline void addMetaData(T *md) {
    if (this->has<T>()) {
      assert(false && "MetaData with key " T::key() " already attached");
    }
    metaFields[T::key()] = md;
  }
  // XXX Meta package

  CgNode(std::string function);
  ~CgNode() {
    for (auto md : metaFields) {
      delete md.second;
    }
  }

  CgNode(const CgNode &other) = delete;
  CgNode(const CgNode &&other) = delete;

  CgNode &operator=(CgNode other) = delete;
  CgNode &operator=(const CgNode &other) = delete;
  CgNode &operator=(CgNode &&other) = delete;

  void addChildNode(CgNodePtr childNode);
  void addParentNode(CgNodePtr parentNode);
  void removeChildNode(CgNodePtr childNode);
  void removeParentNode(CgNodePtr parentNode);
  const CgNodePtrSet &getChildNodes() const;
  const CgNodePtrSet &getParentNodes() const;

  void setReachable(bool reachable = true) { this->reachable = reachable; }
  bool isReachable() const { return this->reachable; }
  bool isSameFunction(CgNodePtr otherNode) const;

  std::string getFunctionName() const;
  int getLineNumber() const { return line; }
  void setLineNumber(int line);
  int getLineNumber() { return line; }
  void setFilename(std::string filename);
  std::string getFilename() { return filename; }

  const CgNodePtrSet &getSpantreeParents() const { return spantreeParents; }

  // PGOE stuff
  unsigned long long getExpectedNumberOfSamples() const;
  int getNumberOfUnwindSteps() const;
  // marker pos & dependent conjunction stuff
  CgNodePtrSet &getMarkerPositions();
  CgNodePtrSet &getDependentConjunctions();
  const CgNodePtrSet &getMarkerPositionsConst() const;
  const CgNodePtrSet &getDependentConjunctionsConst() const;

  // Node states
  void setState(CgNodeState state, int numberOfUnwindSteps = 0);
  CgNodeState getStateRaw() const;
  bool isInstrumented() const;
  bool isInstrumentedWitness() const;
  bool isInstrumentedConjunction() const;
  bool isInstrumentedCallpath() const;
  bool isUnwound() const;
  bool isUnwoundSample() const;
  bool isUnwoundInstr() const;

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

  // note that these metrics are based on a profile and might be pessimistic
  unsigned long long expectedNumberOfSamples;

  bool reachable;

  CgNodePtrSet childNodes;
  CgNodePtrSet parentNodes;

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
  inline bool operator()(const CgNodePtr &lhs, const CgNodePtr &rhs) {
    const auto &[hasLHS, objLHS] = lhs->checkAndGet<pira::BaseProfileData>();
    const auto &[hasRHS, objRHS] = rhs->checkAndGet<pira::BaseProfileData>();
    assert(hasLHS && hasRHS && "For CalledMoreOften struct, both nodes need meta data");
    return objLHS->getNumberOfCalls() < objRHS->getNumberOfCalls();
  }
};
typedef std::priority_queue<CgNodePtr, std::vector<CgNodePtr>, CalledMoreOften> CgNodePtrQueueMostCalls;

struct CgEdge {
  CgNodePtr from;
  CgNodePtr to;

  CgEdge(CgNodePtr from, CgNodePtr to) : from(from), to(to) {}

  inline bool operator<(const CgEdge &other) const { return std::tie(from, to) < std::tie(other.from, other.to); }

  friend bool operator==(const CgEdge &lhs, const CgEdge &rhs) {
    return std::tie(lhs.from, lhs.to) == std::tie(rhs.from, rhs.to);
  }

  friend std::ostream &operator<<(std::ostream &stream, const CgEdge &e) {
    stream << *(e.from) << " -> " << *(e.to);
    return stream;
  }
};

template <typename T, typename... Args>
T *getOrCreateMD(CgNodePtr p, const Args &...args) {
  auto [has, md] = p->checkAndGet<T>();
  if (has) {
    return md;
  } else {
    auto nmd = new T(args...);
    p->addMetaData(nmd);
    return nmd;
  }
}

#endif

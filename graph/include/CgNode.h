/**
 * File: CgNode.h
 * License: Part of the MetaCG project. Licensed under BSD 3 clause license. See LICENSE.txt file at
 * https://github.com/tudasc/metacg/LICENSE.txt
 */
#ifndef METACG_GRAPH_CGNODE_H
#define METACG_GRAPH_CGNODE_H

// clang-format off
// Graph library
#include "MetaData.h"
#include "CgNodePtr.h"

// PGIS library
#include "CgLocation.h"
#include "CgNodeMetaData.h"

// System library
#include <map>
#include <queue>
#include <string>
#include <vector>
// clang-format on

// iterate priority_queue as of: http://stackoverflow.com/a/1385520
template <class T, class S, class C>
S &Container(std::priority_queue<T, S, C> &q) {
  struct HackedQueue : private std::priority_queue<T, S, C> {
    static S &Container(std::priority_queue<T, S, C> &q) { return q.*&HackedQueue::c; }
  };
  return HackedQueue::Container(q);
}

namespace metacg {

enum CgNodeState {
  NONE, // used
  INSTRUMENT_WITNESS, // used
  UNWIND_SAMPLE, // unused
  INSTRUMENT_CONJUNCTION, // unclear
  UNWIND_INSTR, // unused
  INSTRUMENT_CUBE, // unused
  INSTRUMENT_PATH // used
};

/**
 * Node in the call graph with potentially attached metadata.
 */
class CgNode {
  /* MetaData storage */
  using MetaData = metacg::MetaData;
  mutable std::unordered_map<std::string, MetaData *> metaFields;
  /* End MetaData storage */

 public:
  /**
   * Checks if metadata of type #T is attached
   * @tparam T
   * @return
   */
  template <typename T>
  inline bool has() const {
    return metaFields.find(T::key()) != metaFields.end();
  }

  /**
   * Returns pointer to attached metadata of type #T
   * @tparam T
   * @return
   */
  template <typename T>
  inline T *get() const {
    assert(metaFields[T::key()] && "meta field for key exists");
    auto val = metaFields[T::key()];
    return reinterpret_cast<T *>(val);
  }

  /**
   * Checks for attached metadata of type #T
   * @tparam T
   * @return tuple: (wasFound, pointerToMetaData)
   */
  template <typename T>
  inline std::pair<bool, T *> checkAndGet() const {
    if (this->has<T>()) {
      auto bpd = this->get<T>();
      assert(bpd && "meta data attached");
      return {true, bpd};
    }
    return {false, nullptr};
  }

  /**
   * Adds a *new* metadata entry for #T if none exists
   * @tparam T
   * @param md
   */
  template <typename T>
  inline void addMetaData(T *md) {
    //    if (this->has<T>()) {
    //      assert(false && "MetaData with key already attached");
    //    }
    metaFields[T::key()] = md;
  }

  explicit CgNode(std::string function);
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

  /**
  * Compares the function names for equality.
  * @param otherNode
  * @return
  */
  bool isSameFunction(CgNodePtr otherNode) const;

  // Reachable from main
  void setReachable(bool reachable = true) { this->reachable = reachable; }
  // Reachable from main
  bool isReachable() const { return this->reachable; }

  bool isVirtual() const { return isMarkedVirtual; }
  void setIsVirtual(bool virtuality) { isMarkedVirtual = virtuality; }

  bool getHasBody() const { return hasBody; }
  void setHasBody(bool hasBody) { this->hasBody = hasBody; }

  std::string getFunctionName() const;
  int getLineNumber() const { return line; }
  void setLineNumber(int line);
  void setFilename(std::string filename);
  std::string getFilename() { return filename; }

  // Node states (they are specific to PGIS)
  // TODO: Move to some PGIS MetaData
  void setState(CgNodeState state, int numberOfUnwindSteps = 0);
  bool isInstrumented() const;
  bool isInstrumentedWitness() const;
  bool isInstrumentedConjunction() const;
  bool isInstrumentedCallpath() const;
  bool isUnwound() const;
  bool isUnwoundSample() const;
  bool isUnwoundInstr() const;

  /**
   * Mark for instrumentation: this -> parent
   */
  void instrumentFromParent(CgNodePtr parent);
  std::unordered_set<CgNodePtr> getInstrumentedParentEdges() { return this->instrumentedParentEdges; }

  // spanning tree stuff (still relevant?)
  bool isSpantreeParent(CgNodePtr);
  const CgNodePtrSet &getSpantreeParents() const { return spantreeParents; }
  void reset();

  // XXX Still used?
  void updateNodeAttributes(bool updateNumberOfSamples = true);


  bool hasUniqueCallPath() const;
  bool isLeafNode() const;
  bool isRootNode() const;

  void dumpToDot(std::ofstream &outputStream, int procNum);

  void print();

  friend std::ostream &operator<<(std::ostream &stream, const CgNode &n);
  friend std::ostream &operator<<(std::ostream &stream, const CgNodePtrSet &s);

 private:
  std::string functionName;
  CgNodeState state;

  // note that these metrics are based on a profile and might be pessimistic
  unsigned long long expectedNumberOfSamples;

  bool reachable;
  bool isMarkedVirtual;
  bool hasBody;

  CgNodePtrSet childNodes;
  CgNodePtrSet parentNodes;

  std::unordered_set<CgNodePtr> instrumentedParentEdges;

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

}

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
T *getOrCreateMD(CgNodePtr p, const Args &... args) {
  auto [has, md] = p->template checkAndGet<T>();
  if (has) {
    return md;
  } else {
    auto nmd = new T(args...);
    p->template addMetaData(nmd);
    return nmd;
  }
}

#endif

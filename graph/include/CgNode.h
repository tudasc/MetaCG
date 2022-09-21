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
    assert(metaFields[T::key()] && "meta field for key must exist");
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
    if (this->has<T>()) {
      assert(false && "MetaData with key already attached");
    }
    metaFields[T::key()] = md;
  }

  /**
   * Creates a call graph node for a function with name @function.
   * It should not be used directly, instead construct CgNodePtr values.
   * TODO: Currently sets isVirtual and hasBody per default to false. This should be refactored.
   * @param function
   */
  explicit CgNode(std::string function);

  /**
   * CgNode destructs all attached meta data when destructed.
   */
  ~CgNode() {
    for (const auto &md : metaFields) {
      delete md.second;
    }
  }

  /** We delete copy ctor and copy assign op */
  CgNode(const CgNode &other) = delete;
  CgNode(const CgNode &&other) = delete;
  CgNode &operator=(CgNode other) = delete;
  CgNode &operator=(const CgNode &other) = delete;
  CgNode &operator=(CgNode &&other) = delete;

  /**
   * Adds a child node to the vector of children.
   * @param childNode
   */
  void addChildNode(CgNodePtr childNode);
  /**
   * Adds a parent node to the vector of parents.
   * @param parentNode
   */
  void addParentNode(CgNodePtr parentNode);
  /**
   * Removes a child node from the vector of children.
   * @param childNode
   */
  void removeChildNode(CgNodePtr childNode);
  /**
   * Removes a parent node from the vector of parent nodes.
   * @param parentNode
   */
  void removeParentNode(CgNodePtr parentNode);
  /**
   * @return the set of children of this node.
   */
  const CgNodePtrSet &getChildNodes() const;
  /**
   *
   * @return the set of parents of this node.
   */
  const CgNodePtrSet &getParentNodes() const;

  /**
  * Compares the function names for equality.
  * @param otherNode
  * @return
  */
  bool isSameFunction(CgNodePtr otherNode) const;

  bool isVirtual() const { return isMarkedVirtual; }
  void setIsVirtual(bool virtuality) { isMarkedVirtual = virtuality; }

  bool getHasBody() const { return hasBody; }
  void setHasBody(bool hasBody) { this->hasBody = hasBody; }

  /**
   *
   * @return the name of the function
   */
  std::string getFunctionName() const;

  /**
   * Mark for instrumentation: this -> parent
   */
  void instrumentFromParent(CgNodePtr parent);
  std::unordered_set<CgNodePtr> getInstrumentedParentEdges() { return this->instrumentedParentEdges; }

  bool isLeafNode() const;
  bool isRootNode() const;

  void dumpToDot(std::ofstream &outputStream, int procNum);

  void print();

  friend std::ostream &operator<<(std::ostream &stream, const CgNode &n);
  friend std::ostream &operator<<(std::ostream &stream, const CgNodePtrSet &s);

 private:
  std::string functionName;

  bool isMarkedVirtual;
  bool hasBody;

  CgNodePtrSet childNodes;
  CgNodePtrSet parentNodes;

  std::unordered_set<CgNodePtr> instrumentedParentEdges;
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

/**
 * Convenience function to get the existing meta data node, or attach a new meta data node.
 * TODO: Should this return a pair<bool, T*> instead with first indicating if newly inserted?
 *
 * @tparam T
 * @tparam Args
 * @param p
 * @param args Arguments to construct MetaData  T, ignored when MetaData already exists.
 * @return
 */
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

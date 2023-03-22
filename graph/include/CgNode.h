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
#include <string>
#include <utility>
#include <vector>
// clang-format on

namespace metacg {

class CgNode {
 public:
  /**
   * Checks if metadata of type #T is attached
   * @tparam T
   * @return
   */
  template <typename T>
  inline bool has() const {
    return metaFields.find(T::key) != metaFields.end();
  }

  /**
   * Returns pointer to attached metadata of type #T
   * @tparam T
   * @return
   */
  template <typename T>
  inline T *get() const {
    assert(metaFields.count(T::key) > 0 && "meta field for key must exist");
    auto val = metaFields.at(T::key);
    return static_cast<T *>(val);
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
    metaFields[T::key] = md;
  }

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
  T *getOrCreateMD(const Args &...args) {
    auto [has, md] = this->template checkAndGet<T>();
    if (has) {
      return md;
    } else {
      auto nmd = new T(args...);
      this->template addMetaData(nmd);
      return nmd;
    }
  }

  /**
   * Creates a call graph node for a function with name @function.
   * It should not be used directly, instead construct CgNodePtr values.
   * TODO: Currently sets isVirtual and hasBody per default to false. This should be refactored.
   * @param function
   */
  explicit CgNode(std::string function, bool isVirtual = false, bool hasBody = false)
      : id(std::hash<std::string>()(function)),
        functionName(std::move(function)),
        isMarkedVirtual(isVirtual),
        hasBody(hasBody){};

  /**
   * CgNode destructs all attached meta data when destructed.
   */
  ~CgNode();

  /** We delete copy ctor and copy assign op */
  CgNode(const CgNode &other) = delete;
  CgNode(const CgNode &&other) = delete;
  CgNode &operator=(CgNode other) = delete;
  CgNode &operator=(const CgNode &other) = delete;
  CgNode &operator=(CgNode &&other) = delete;
  bool operator==(const CgNode &otherNode) const;

  bool isSameFunction(const CgNode &otherNode) const;

  /**
   * Compares the function names for equality.
   * @param otherNode
   * @return
   */
  bool isSameFunctionName(const CgNode &otherNode) const;

  bool isVirtual() const { return isMarkedVirtual; }
  void setIsVirtual(bool virtuality) { isMarkedVirtual = virtuality; }

  bool getHasBody() const { return hasBody; }
  void setHasBody(bool bodyStatus) { hasBody = bodyStatus; }

  /**
   *
   * @return the nameIdMap of the function
   */
  std::string getFunctionName() const;

  void dumpToDot(std::ofstream &outputStream, int procNum);

  void print();

  size_t getId() const { return id; }

  friend std::ostream &operator<<(std::ostream &stream, const CgNode &n);

  std::unordered_map<std::string, MetaData *> &getMetaDataContainer() { return metaFields; }

  void setMetaDataContainer(std::unordered_map<std::string, MetaData *> data) { metaFields = std::move(data); }

 private:
  size_t id = -1;
  std::string functionName;
  std::unordered_map<std::string, MetaData *> metaFields;
  bool isMarkedVirtual;
  bool hasBody;
};

}  // namespace metacg
#endif

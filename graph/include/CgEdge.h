/**
* File: CgEdge.h
* License: Part of the MetaCG project. Licensed under BSD 3 clause license. See LICENSE.txt file at
* https://github.com/tudasc/metacg/LICENSE.txt
*/

#ifndef GRAPHLIB_CGEDGE_H
#define GRAPHLIB_CGEDGE_H

//Project Includes
#include "MetaData.h"

//standard library includes
#include <cassert>
#include <string>
#include <unordered_map>


namespace metacg {
class CgNode;

class CgEdge {
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
    metaFields[T::key()] = md;
  }

  const metacg::CgNode &getSource();
  const metacg::CgNode &getTarget();

  CgEdge(const metacg::CgNode &source,const metacg::CgNode &target);
  ~CgEdge();
  /** We delete copy ctor and copy assign op */
  CgEdge(const CgEdge &other) = delete;
  CgEdge(const CgEdge &&other) = delete;
  CgEdge &operator=(CgEdge other) = delete;
  CgEdge &operator=(const CgEdge &other) = delete;
  CgEdge &operator=(CgEdge &&other) = delete;

 private:
  const metacg::CgNode &source;
  const metacg::CgNode &target;
  std::unordered_map<std::string, metacg::MetaData *> metaFields;
};
}  // namespace metacg
#endif  // GRAPHLIB_CGEDGE_H
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

  inline bool has(const MetaData *const md) const { return metaFields.find(md->getKey()) != metaFields.end(); }

  inline bool has(const std::string &metadataName) const { return metaFields.find(metadataName) != metaFields.end(); }

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

  inline MetaData *get(const std::string &metadataName) const {
    assert(metaFields.count(metadataName) > 0 && "meta field for key must exist");
    auto val = metaFields.at(metadataName);
    return val;
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

  inline void addMetaData(MetaData *const md) {
    if (this->has(md->getKey())) {
      assert(false && "MetaData with key already attached");
    }
    metaFields[md->getKey()] = md;
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

  /**
   * Compares the function for equality by their node id.
   * @param otherNode
   * @return
   */
  bool isSameFunction(const CgNode &otherNode) const;

  /**
   * Compares the function names for equality.
   * @param otherNode
   * @return
   */
  bool isSameFunctionName(const CgNode &otherNode) const;

  /**
   * Check whether the function is marked virtual.
   * @param
   * @return the virtuality status
   */
  bool isVirtual() const { return isMarkedVirtual; }

  /**
   * Set the virtuality for the function.
   * @param virtuality - the new status of the function
   * @return
   */
  void setIsVirtual(bool virtuality) { isMarkedVirtual = virtuality; }

  /**
   * Check whether a function body has been found.
   * @param
   * @return
   */
  bool getHasBody() const { return hasBody; }

  /**
   * Mark function to have a body found
   * @param bodyStatus - the new status of the node
   * @return
   */
  void setHasBody(bool bodyStatus) { hasBody = bodyStatus; }

  /**
   * @return the nameIdMap of the function
   */
  std::string getFunctionName() const;

  /**
   * Get the id of the function node
   *
   * @return the id of the function node
   */
  size_t getId() const { return id; }

  friend std::ostream &operator<<(std::ostream &stream, const CgNode &n);

  /**
   * Get the whole container of all attached metadata with its unique name identifier
   *
   * @return a map, mapping the name of the metadata to a metadata pointer
   */
  const std::unordered_map<std::string, MetaData *> &getMetaDataContainer() const { return metaFields; }

  /**
   * Override the current set of all node attached metadata with a new set
   *
   * @param data - a map, mapping the name of the metadata to a metadata pointer
   */
  void setMetaDataContainer(std::unordered_map<std::string, MetaData *> data) { metaFields = std::move(data); }

 private:
  size_t id = -1;
  std::string functionName;
  std::unordered_map<std::string, MetaData *> metaFields;
  bool isMarkedVirtual;
  bool hasBody;
};

}  // namespace metacg

namespace nlohmann {

template <typename T>
struct adl_serializer<std::unique_ptr<T>> {
  // allow nlohmann to serialize unique pointer
  // as json does not have a representation of a pointer construct
  // we only generate the value of the object pointed to
  static void to_json(json &j, const std::unique_ptr<T> &uniquePointerT) {
    if (uniquePointerT) {
      j = *uniquePointerT;
    } else {
      j = nullptr;
    }
  }

  static void from_json(const json &j, std::unique_ptr<T> &uniquePointerT) {
    if (j.is_null()) {
      uniquePointerT = nullptr;
    } else {
      uniquePointerT.reset(j.get<T>());
    }
  }
};

template <>
struct adl_serializer<std::unordered_map<std::string, metacg::MetaData *>> {
  // interestingly enough, j.dump returns an empty string, but this function gets called and works
  static std::unordered_map<std::string, metacg::MetaData *> from_json(const json &j) {
    // use compound type serialization instead of metadata serialization,
    // because we need access to key and value for metadata creation
    std::unordered_map<std::string, metacg::MetaData *> metadataAccumulator;
    metadataAccumulator.reserve(j.size());
    for (const auto &elem : j.items()) {
      // logging of generation failure is done in create<> function, no else needed
      // if metadata can not be fully generated, there will not be a stub key generated for it in the memory
      // representation
      if (auto obj = metacg::MetaData::create<>(elem.key(), elem.value()); obj != nullptr) {
        metadataAccumulator[elem.key()] = obj;
      }
    }
    return metadataAccumulator;
  }

  // we need to fully specialize the adl_serializer
  static void to_json(json &j, const std::unordered_map<std::string, metacg::MetaData *> &t) {
    json jsonAccumulator;
    for (const auto &elem : t) {
      // if metadata_json can not be fully generated, there will not be a stub key generated for it in the json file
      if (auto generated = elem.second->to_json(); !generated.empty()) {
        jsonAccumulator[elem.first] = generated;
      }
    }
    j = jsonAccumulator;
  }
};

// place CgNodePtr de/serialization here instead of CgNodePtr.h
// because we need full access underlying datastructure CgNode
template <>
struct adl_serializer<CgNodePtr> {
  static CgNodePtr from_json(const json &j) {
    CgNodePtr cgNode = nullptr;
    if (j.is_null()) {
      return cgNode;
    }
    cgNode = std::make_unique<metacg::CgNode>(j.at("FunctionName"));
    cgNode->setIsVirtual(j.at("IsVirtual").get<bool>());
    cgNode->setHasBody(j.at("HasBody").get<bool>());
    cgNode->setMetaDataContainer(j.at("MetaData"));
    return cgNode;
  }
  static void to_json(json &j, const CgNodePtr &t) {
    j = {{"FunctionName", t->getFunctionName()},
         {"IsVirtual", t->isVirtual()},
         {"HasBody", t->getHasBody()},
         {"MetaData", t->getMetaDataContainer()}};
  }
};

}  // namespace nlohmann

#endif

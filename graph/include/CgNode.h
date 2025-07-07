/**
 * File: CgNode.h
 * License: Part of the MetaCG project. Licensed under BSD 3 clause license. See LICENSE.txt file at
 * https://github.com/tudasc/metacg/LICENSE.txt
 */
#ifndef METACG_GRAPH_CGNODE_H
#define METACG_GRAPH_CGNODE_H

// clang-format off

// Graph library
#include "CgTypes.h"
#include "metadata/MetaData.h"

// System library
#include <map>
#include <string>
#include <utility>
#include <memory>
#include <optional>
// clang-format on

namespace metacg {

class CgNode;
class Callgraph;

class CgNode {
  friend class Callgraph;
 private:
  /**
   * Creates a call graph node for a function with name @function.
   * Cannot be invoked directly, use CallGraph::insert instead.
   * @param function
   */
  explicit CgNode(NodeId id, std::string function, std::optional<std::string> origin, bool isVirtual, bool hasBody);

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

  inline bool has(const std::string& metadataName) const { return metaFields.find(metadataName) != metaFields.end(); }

  /**
   * Returns pointer to attached metadata of type #T
   * @tparam T
   * @return
   */
  template <typename T>
  inline T* get() const {
//    assert(metaFields.count(T::key) > 0 && "meta field for key must exist");
    if (auto it = metaFields.find(T::key); it != metaFields.end()) {
      return static_cast<T*>(it->second.get());
    }
    return nullptr;
  }

  inline MetaData* get(const std::string& metadataName) const {
//    assert(metaFields.count(metadataName) > 0 && "meta field for key must exist");
    if (auto it = metaFields.find(metadataName); it != metaFields.end()) {
      return it->second.get();
    }
   return nullptr;
  }

  /**
   * Adds a *new* metadata entry for #T if none exists
   * @tparam T
   * @param md
   */
  template <typename T>
  inline void addMetaData(std::unique_ptr<T> md) {
    assert(md && "Cannot add null metadata");
    assert(!this->has<T>() && "MetaData with key already attached");
    metaFields[T::key] = std::move(md);
  }

  template <typename T, typename... Args>
  inline void addMetaData(Args&&... args) {
    assert(!this->has<T>() && "MetaData with key already attached");
    metaFields[T::key] = std::make_unique<T>(std::forward(args)...);
  }

  inline void addMetaData(std::unique_ptr<MetaData> md) {
    assert(md && "Cannot add null metadata");
    assert(!this->has(md->getKey()) && "MetaData with key already attached");
    metaFields[md->getKey()] = std::move(md);
  }

  /**
   * Convenience function to get the existing meta data node, or attach a new meta data node.
   *
   * @tparam T
   * @tparam Args
   * @param p
   * @param args Arguments to construct MetaData  T, ignored when MetaData already exists.
   * @return
   */
  template <typename T, typename... Args>
  T& getOrCreateMD(const Args&... args) {
    auto& md = metaFields[T::key];
    if (md) {
      return static_cast<T&>(*md);
    }
    md = std::make_unique<T>(args...);
    return static_cast<T&>(*md);
  }

  ~CgNode() = default;

  /** We delete copy ctor and copy assign op */
  CgNode(const CgNode& other) = delete;
  CgNode(const CgNode&& other) = delete;
  CgNode& operator=(CgNode other) = delete;
  CgNode& operator=(const CgNode& other) = delete;
  CgNode& operator=(CgNode&& other) = delete;

  /**
   * Compares the function names for equality.
   * @param otherNode
   * @return
   */
  bool isSameFunctionName(const CgNode& otherNode) const { return functionName == otherNode.getFunctionName(); }

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
  std::string getFunctionName() const { return functionName; }

  /**
   * Sets the function name.
   * @param name
   */
  void setFunctionName(std::string name) {
    this->functionName = std::move(name);
  }

  [[deprecated("Attach \"OverrideMD\" instead")]] void setIsVirtual(bool);

  [[deprecated("Check with has<OverrideMD>() instead")]] bool isVirtual();

  /**
   * Returns the origin.
   * @return
   */
  std::optional<std::string> getOrigin() const { return origin; }

  void setOrigin(std::optional<std::string> origin) {
    this->origin = std::move(origin);
  }

  /**
   * Get the id of the function node
   *
   * @return the id of the function node
   */
  NodeId getId() const { return id; }

  friend std::ostream& operator<<(std::ostream& stream, const CgNode& n);

  /**
   * Get the whole container of all attached metadata with its unique name identifier
   *
   * @return a map, mapping the name of the metadata to a metadata pointer
   */
  const std::unordered_map<std::string, std::unique_ptr<MetaData>>& getMetaDataContainer() const { return metaFields; }

  /**
   * Override the current set of all node attached metadata with a new set
   *
   * @param data - a map, mapping the name of the metadata to a metadata pointer
   */
  void setMetaDataContainer(std::unordered_map<std::string, std::unique_ptr<MetaData>> data) { metaFields = std::move(data); }

 public:
  const NodeId id;

 private:
  std::string functionName;
  std::optional<std::string> origin;
  bool hasBody;
  std::unordered_map<std::string, std::unique_ptr<MetaData>> metaFields;
};

}  // namespace metacg

#endif

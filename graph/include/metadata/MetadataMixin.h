/**
 * File: MetaData.h
 * License: Part of the MetaCG project. Licensed under BSD 3 clause license. See LICENSE.txt file at
 * https://github.com/tudasc/metacg/LICENSE.txt
 */
#ifndef METACG_METADATAMIXIN_H
#define METACG_METADATAMIXIN_H

#include "metadata/MetaData.h"

#include <memory>
#include <string>
#include <unordered_map>

namespace metacg {

/**
 * This is a mixin used to extend class with the capability to attach and manage metadata objects.
 */
class MetadataMixin {
 public:
  MetadataMixin() = default;

  // Delete copy constructor and assignment.
  MetadataMixin(const MetadataMixin&) = delete;
  MetadataMixin& operator=(const MetadataMixin&) = delete;

  // Support moving
  MetadataMixin(MetadataMixin&&) noexcept = default;
  MetadataMixin& operator=(MetadataMixin&&) noexcept = default;

  ~MetadataMixin() = default;

  /**
   * Checks if metadata of type #T is attached
   * @tparam T
   * @return
   */
  template <typename T>
  bool has() const {
    return metaFields.find(T::key) != metaFields.end();
  }

  bool has(const std::string& metadataName) const { return metaFields.find(metadataName) != metaFields.end(); }

  /**
   * Returns pointer to attached metadata of type #T
   * @tparam T
   * @return The metadata or null, if no metadata of type #T exists.
   */
  template <typename T>
  T* get() const {
    if (auto it = metaFields.find(T::key); it != metaFields.end()) {
      return static_cast<T*>(it->second.get());
    }
    return nullptr;
  }

  MetaData* get(const std::string& metadataName) const {
    if (auto it = metaFields.find(metadataName); it != metaFields.end()) {
      return it->second.get();
    }
    return nullptr;
  }

  /**
   * Erases the metadata of given type.
   * @tparam T
   * @return True if there was metadata of this type, false otherwise.
   */
  template <typename T>
  bool erase() {
    return metaFields.erase(T::key);
  }

  /**
   * Erases the metadata with the given name.
   * @return True if there was metadata with this name, false otherwise.
   */
  bool erase(const std::string& mdKey) { return metaFields.erase(mdKey); }

  /**
   * Adds a metadata entry of type #T. Overrides any existing metadata of this type.
   * @tparam T
   * @param md
   */
  template <typename T>
  void addMetaData(std::unique_ptr<T> md) {
    assert(md && "Cannot add null metadata");
    metaFields[T::key] = std::move(md);
  }

  /**
   * Creates and adds a metadata entry of type #T. Overrides any existing metadata of this type.
   * @tparam T
   * @tparam Args
   * @param args Arguments passed to the constructor of #T
   */
  template <typename T, typename... Args>
  void addMetaData(Args&&... args) {
    metaFields[T::key] = std::make_unique<T>(std::forward(args)...);
  }

  /**
   * Adds the given metadata. Overrides any existing metadata of the same type.
   * @param md
   */
  void addMetaData(std::unique_ptr<MetaData> md) {
    assert(md && "Cannot add null metadata");
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
  T& getOrCreate(const Args&... args) {
    auto& md = metaFields[T::key];
    if (md) {
      return static_cast<T&>(*md);
    }
    md = std::make_unique<T>(args...);
    return static_cast<T&>(*md);
  }

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
  void setMetaDataContainer(std::unordered_map<std::string, std::unique_ptr<MetaData>> data) {
    metaFields = std::move(data);
  }

 private:
  std::unordered_map<std::string, std::unique_ptr<MetaData>> metaFields;
};

}  // namespace metacg

#endif  // METACG_METADATAMIXIN_H

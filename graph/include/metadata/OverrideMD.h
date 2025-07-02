/**
 * File: OverrideMD.h
 * License: Part of the MetaCG project. Licensed under BSD 3 clause license. See LICENSE.txt file at
 * https://github.com/tudasc/metacg/LICENSE.txt
 */

#ifndef METACG_OVERRIDEMD_H
#define METACG_OVERRIDEMD_H

#include "LoggerUtil.h"
#include "metadata/MetaData.h"
#include <vector>

/**
 * Copies items from the source vector into the destination vector. Removes duplicates.
 * @tparam T Vector type
 * @param dst Destination vector
 * @param src Source vector
 */
template <typename T>
void mergeVectors(std::vector<T>& dst, const std::vector<T>& src) {
  for (const auto& entry : src) {
    if (auto it = std::find(dst.begin(), dst.end(), entry); it == dst.end()) {
      dst.push_back(entry);
    }
  }
}

/**
 *  This is first-party metadata that allows a user to attach override data to function nodes
 *  MetaCG tries to generate this metadata whenever legacy file formats are read.
 *  It stores information about whether a function is virtual, or whether it overrides,
 *  and which function it overrides or gets overridden by
 *  The deprecated setIsVirtual and IsVirtual use this metadata to achieve compatibility with older API calls
 */
struct OverrideMD final : metacg::MetaData::Registrar<OverrideMD> {
 public:
  static constexpr const char* key = "overrideMD";
  OverrideMD() = default;

  explicit OverrideMD(const nlohmann::json& j) {
    overrides = j.at("overrides").get<std::vector<size_t>>();
    overriddenBy = j.at("overriddenBy").get<std::vector<size_t>>();
  }

  nlohmann::json to_json() const final { return {{"overrides", overrides}, {"overriddenBy", overriddenBy}}; }

 private:
  OverrideMD(const OverrideMD& other) : overrides(other.overrides), overriddenBy(other.overriddenBy) {}

 public:
  void merge(const MetaData& toMerge) final {
    assert(toMerge.getKey() == getKey() && "Trying to merge OverrideMD with meta data of different types");

    const OverrideMD* toMergeDerived = static_cast<const OverrideMD*>(&toMerge);

    mergeVectors(overriddenBy, toMergeDerived->overriddenBy);
    mergeVectors(overrides, toMergeDerived->overrides);
  }

  std::unique_ptr<MetaData> clone() const final { return std::unique_ptr<MetaData>(new OverrideMD(*this)); }

  const char* getKey() const final { return key; }

  std::vector<size_t> overrides;
  std::vector<size_t> overriddenBy;
};
#endif  // METACG_OVERRIDEMD_H

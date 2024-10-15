/**
 * File: OverrideMD.h
 * License: Part of the MetaCG project. Licensed under BSD 3 clause license. See LICENSE.txt file at
 * https://github.com/tudasc/metacg/LICENSE.txt
 */

#ifndef METACG_OVERRIDEMD_H
#define METACG_OVERRIDEMD_H
#include "MetaData.h"
#include <vector>

/**
 *  This is first-party metadata that allows a user to attach override data to function nodes
 *  MetaCG tries to generate this metadata whenever legacy file formats are read.
 *  It stores information about whether a function is virtual, or whether it overrides,
 *  and which function it overrides or gets overridden by
 *  The deprecated setIsVirtual and IsVirtual use this metadata to achieve compatibility with older API calls
 */
struct OverrideMetadata final : metacg::MetaData::Registrar<OverrideMetadata> {
 public:
  static constexpr const char* key = "overrideMD";
  OverrideMetadata() = default;

  explicit OverrideMetadata(const nlohmann::json& j) {
    overrides = j.at("overrides").get<std::vector<size_t>>();
    overriddenBy = j.at("overriddenBy").get<std::vector<size_t>>();
  }

  nlohmann::json to_json() const final { return {{"overrides", overrides}, {"overriddenBy", overriddenBy}}; };

  const char* getKey() const final { return key; }

  std::vector<size_t> overrides;
  std::vector<size_t> overriddenBy;
};
#endif  // METACG_OVERRIDEMD_H

/**
 * File: OverrideMD.h
 * License: Part of the MetaCG project. Licensed under BSD 3 clause license. See LICENSE.txt file at
 * https://github.com/tudasc/metacg/LICENSE.txt
 */

#ifndef METACG_OVERRIDEMD_H
#define METACG_OVERRIDEMD_H

#include "CgNode.h"
#include "LoggerUtil.h"
#include "metadata/MetaData.h"
#include <vector>

namespace metacg {

/**
 * Copies items from the source vector into the destination vector. Removes duplicates.
 * @param dst Destination vector
 * @param src Source vector
 */
inline void mergeVectors(std::vector<NodeId>& dst, const std::vector<NodeId>& src, const GraphMapping& mapping) {
  for (const auto& entry : src) {
    assert(mapping.count(entry) == 1 && "Mapping must exist");
    auto& mappedId = mapping.at(entry);
    if (auto it = std::find(dst.begin(), dst.end(), mappedId); it == dst.end()) {
      dst.push_back(mappedId);
    }
  }
}

/**
 *  This is first-party metadata that allows a user to attach override data to function nodes
 *  MetaCG tries to generate this metadata whenever legacy file formats are read.
 *  It stores information about whether a function is virtual, or whether it overrides,
 *  and which function it overrides or gets overridden by
 *  The deprecated setVirtual and IsVirtual use this metadata to achieve compatibility with older API calls
 */
struct OverrideMD final : metacg::MetaData::Registrar<OverrideMD> {
 public:
  static constexpr const char* key = "overrideMD";
  OverrideMD() = default;

  explicit OverrideMD(const nlohmann::json& j, StrToNodeMapping& strToNode) {
    auto overrideStrs = j.at("overrides").get<std::vector<std::string>>();
    for (auto& s : overrideStrs) {
      auto* n = strToNode.getNodeFromStr(s);
      assert(n && "Node is null");
      overrides.push_back(n->getId());
    }

    auto overriddenByStrs = j.at("overriddenBy").get<std::vector<std::string>>();
    for (auto& s : overriddenByStrs) {
      auto* n = strToNode.getNodeFromStr(s);
      assert(n && "Node is null");
      overriddenBy.push_back(n->getId());
    }
  }

  nlohmann::json toJson(NodeToStrMapping& nodeToStr) const final {
    auto jOverrides = nlohmann::json::array();
    for (auto& n : overrides) {
      jOverrides.push_back(nodeToStr.getStrFromNode(n));
    }
    auto jOverriddenBy = nlohmann::json::array();
    for (auto& n : overriddenBy) {
      jOverriddenBy.push_back(nodeToStr.getStrFromNode(n));
    }
    return {{"overrides", jOverrides}, {"overriddenBy", jOverriddenBy}};
  }

 private:
  OverrideMD(const OverrideMD& other) : overrides(other.overrides), overriddenBy(other.overriddenBy) {}

 public:
  void merge(const MetaData& toMerge, const MergeAction&, const GraphMapping& mapping) final {
    assert(toMerge.getKey() == getKey() && "Trying to merge OverrideMD with meta data of different types");

    const OverrideMD* toMergeDerived = static_cast<const OverrideMD*>(&toMerge);

    mergeVectors(overriddenBy, toMergeDerived->overriddenBy, mapping);
    mergeVectors(overrides, toMergeDerived->overrides, mapping);
  }

  std::unique_ptr<MetaData> clone() const final { return std::unique_ptr<MetaData>(new OverrideMD(*this)); }

  void applyMapping(const GraphMapping& mapping) override {
    for (auto& overridesId : overrides) {
      assert(mapping.count(overridesId) == 1 && "Incomplete mapping");
      overridesId = mapping.at(overridesId);
    }
    for (auto& overriddenById : overriddenBy) {
      assert(mapping.count(overriddenById) == 1 && "Incomplete mapping");
      overriddenById = mapping.at(overriddenById);
    }
  }

  const char* getKey() const final { return key; }

  std::vector<NodeId> overrides;
  std::vector<NodeId> overriddenBy;
};

}  // namespace metacg
#endif  // METACG_OVERRIDEMD_H

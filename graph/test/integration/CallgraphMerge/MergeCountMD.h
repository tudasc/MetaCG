/**
 * File: MergeCountMD.h
 * License: Part of the MetaCG project. Licensed under BSD 3 clause license. See LICENSE.txt file at
 * https://github.com/tudasc/metacg/LICENSE.txt
 */
#ifndef METACG_MERGECOUNTMD_H
#define METACG_MERGECOUNTMD_H

#include "metacg/metadata/MetaData.h"

class MergeCountMD : public metacg::MetaData::Registrar<MergeCountMD> {
 public:
  static constexpr const char* key = "mergeCount";
  MergeCountMD() = default;
  explicit MergeCountMD(const nlohmann::json& j, metacg::StrToNodeMapping&) {
    metacg::MCGLogger::logTrace("Reading MergeCountMD from json");
    if (j.is_null()) {
      metacg::MCGLogger::logErrorUnique("Could not retrieve meta data for {}", "MergeCountMD");
      return;
    }
    // Legacy files store the count directly instead of wrapping it in an object
    this->mergeCount = j.is_object() ? j.at("count").get<int>() : j.get<int>();
  }

 private:
  MergeCountMD(const MergeCountMD& other) : mergeCount(other.mergeCount) {}

 public:
  nlohmann::json toJson(metacg::NodeToStrMapping&) const override { return {{"count", mergeCount}}; }

  const char* getKey() const override { return key; }

  void merge(const MetaData& toMerge, std::optional<metacg::MergeAction>, const metacg::GraphMapping&) final {
    assert(toMerge.getKey() == getKey() && "Trying to merge MergeCountMD with meta data of different types");

    const MergeCountMD* toMergeDerived = static_cast<const MergeCountMD*>(&toMerge);

    this->mergeCount += toMergeDerived->mergeCount + 1;
  }

  void applyMapping(const metacg::GraphMapping&) override {}

  std::unique_ptr<MetaData> clone() const final { return std::unique_ptr<MetaData>(new MergeCountMD(*this)); }

 private:
  int mergeCount{0};
};

#endif  // METACG_MERGECOUNTMD_H

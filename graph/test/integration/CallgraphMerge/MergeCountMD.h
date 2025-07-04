/**
 * File: MergeCountMD.h
 * License: Part of the MetaCG project. Licensed under BSD 3 clause license. See LICENSE.txt file at
 * https://github.com/tudasc/metacg/LICENSE.txt
 */
#ifndef METACG_MERGECOUNTMD_H
#define METACG_MERGECOUNTMD_H

#include "metadata/MetaData.h"

class MergeCountMD : public metacg::MetaData::Registrar<MergeCountMD> {
 public:
  static constexpr const char* key = "mergeCount";
  MergeCountMD() = default;
  explicit MergeCountMD(const nlohmann::json& j) {
    metacg::MCGLogger::instance().getConsole()->trace("Reading MergeCountMD from json");
    if (j.is_null()) {
      metacg::MCGLogger::instance().getConsole()->trace("Could not retrieve meta data for {}", "MergeCountMD");
      return;
    }
    auto val = j.get<int>();
    this->mergeCount = val;
  }

 private:
  MergeCountMD(const MergeCountMD& other) : mergeCount(other.mergeCount) {}

 public:
  nlohmann::json toJson() const final { return mergeCount; }

  const char* getKey() const override { return key; }

  void merge(const MetaData& toMerge, const MergeAction&, const GraphMapping&) final {
    assert(toMerge.getKey() == getKey() && "Trying to merge MergeCountMD with meta data of different types");

    const MergeCountMD* toMergeDerived = static_cast<const MergeCountMD*>(&toMerge);

    this->mergeCount += toMergeDerived->mergeCount + 1;
  }

  std::unique_ptr<MetaData> clone() const final { return std::unique_ptr<MetaData>(new MergeCountMD(*this)); }

 private:
  int mergeCount{0};
};

#endif  // METACG_MERGECOUNTMD_H

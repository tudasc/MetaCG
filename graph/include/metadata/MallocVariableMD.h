/**
* File: MallocVariableMD.h
* License: Part of the MetaCG project. Licensed under BSD 3 clause license. See LICENSE.txt file at
* https://github.com/tudasc/metacg/LICENSE.txt
 */
#ifndef CGCOLLECTOR2_MALLOCVARIABLEMD_H
#define CGCOLLECTOR2_MALLOCVARIABLEMD_H

#include "metadata/MetaData.h"

namespace metacg {

class MallocVariableMD : public metacg::MetaData::Registrar<MallocVariableMD> {
 public:
  std::map<std::string, std::string> allocs;

  static constexpr const char* key = "mallocCollector";

  MallocVariableMD() = default;

  explicit MallocVariableMD(const nlohmann::json& j, StrToNodeMapping&) {
    metacg::MCGLogger::instance().getConsole()->trace("Creating {} metadata from json", key);
    if (j.is_null()) {
      metacg::MCGLogger::instance().getConsole()->trace("Could not retrieve metadata for {}", key);
    }

    for (const auto& elem : j) {
      allocs[elem.at("global")] = elem.at("allocStmt");
    }
  }

 private:
  MallocVariableMD(const MallocVariableMD& other) : allocs(other.allocs) {}

 public:
  nlohmann::json toJson(NodeToStrMapping&) const final {
    std::vector<nlohmann::json> jArray;
    jArray.reserve(allocs.size());
    for (const auto& [k, v] : allocs) {
      jArray.push_back({{"global", k}, {"allocStmt", v}});
    }

    return jArray;
  }

  const char* getKey() const override { return key; }

  void merge(const MetaData& toMerge, std::optional<MergeAction>, const GraphMapping&) final {
    assert(toMerge.getKey() == getKey() && "Trying to merge MallocVariableMD with meta data of different types");
    // TODO: Merge not implemented as of now
  }

  std::unique_ptr<MetaData> clone() const final { return std::unique_ptr<MetaData>(new MallocVariableMD(*this)); }

  void applyMapping(const GraphMapping&) override {}
};

}  // namespace metacg

#endif  // CGCOLLECTOR2_MALLOCVARIABLEMD_H

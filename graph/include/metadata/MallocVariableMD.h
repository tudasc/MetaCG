/**
* File: MallocVariableMD.h
* License: Part of the MetaCG project. Licensed under BSD 3 clause license. See LICENSE.txt file at
* https://github.com/tudasc/metacg/LICENSE.txt
 */
#ifndef CGCOLLECTOR2_MALLOCVARIABLEMD_H
#define CGCOLLECTOR2_MALLOCVARIABLEMD_H

#include "metadata/MetaData.h"

class MallocVariableMD : public metacg::MetaData::Registrar<MallocVariableMD> {
 public:
  std::map<std::string, std::string> allocs;

  static constexpr const char* key = "mallocCollector";

  MallocVariableMD() = default;

  explicit MallocVariableMD(const nlohmann::json& j) {
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
  nlohmann::json to_json() const final {
    std::vector<nlohmann::json> jArray;
    jArray.reserve(allocs.size());
    for (const auto& [k, v] : allocs) {
      jArray.push_back({{"global", k}, {"allocStmt", v}});
    }

    return jArray;
  }

  virtual const char* getKey() const { return key; }

  void merge(const MetaData& toMerge) final {
    assert(toMerge.getKey() == getKey() && "Trying to merge MallocVariableMD with meta data of different types");

    const MallocVariableMD* toMergeDerived = static_cast<const MallocVariableMD*>(&toMerge);

    // TODO: Merge not implemented as of now
  }

  MetaData* clone() const final { return new MallocVariableMD(*this); }
};

#endif  // CGCOLLECTOR2_MALLOCVARIABLEMD_H

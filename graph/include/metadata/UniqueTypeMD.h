/**
* File: UniqueTypeMD.h
* License: Part of the MetaCG project. Licensed under BSD 3 clause license. See LICENSE.txt file at
* https://github.com/tudasc/metacg/LICENSE.txt
 */
#ifndef CGCOLLECTOR2_UNIQUETYPEMD_H
#define CGCOLLECTOR2_UNIQUETYPEMD_H

#include "metadata/MetaData.h"

namespace metacg {

class UniqueTypeMD : public metacg::MetaData::Registrar<UniqueTypeMD> {
 public:
  static constexpr const char* key = "uniqueTypeMetaData";

  UniqueTypeMD() = default;

  explicit UniqueTypeMD(const nlohmann::json& j, StrToNodeMapping&) {
    metacg::MCGLogger::instance().getConsole()->trace("Creating {} metadata from json", key);
    if (j.is_null()) {
      metacg::MCGLogger::instance().getConsole()->trace("Could not retrieve metadata for {}", key);
    }
    numTypes = j;
  }

 private:
  UniqueTypeMD(const UniqueTypeMD& other) : numTypes(other.numTypes) {}

 public:
  nlohmann::json toJson(NodeToStrMapping&) const final { return numTypes; }

  const char* getKey() const override { return key; }

  void merge(const MetaData& toMerge, std::optional<MergeAction>, const GraphMapping&) final {
    assert(toMerge.getKey() == getKey() && "Trying to merge UniqueTypeMD with meta data of different types");

    const UniqueTypeMD* toMergeDerived = static_cast<const UniqueTypeMD*>(&toMerge);

    if (numTypes != toMergeDerived->numTypes) {
      numTypes += toMergeDerived->numTypes;

      if (numTypes != 0 && toMergeDerived->numTypes != 0) {
        metacg::MCGLogger::instance().getErrConsole()->warn(
            "Same function defined with different number of types found on merge.");
      }
    }
  }

  std::unique_ptr<MetaData> clone() const final { return std::unique_ptr<MetaData>(new UniqueTypeMD(*this)); }

  void applyMapping(const GraphMapping&) override {}

  int numTypes{0};
};
}  // namespace metacg

#endif  // CGCOLLECTOR2_UNIQUETYPEMD_H

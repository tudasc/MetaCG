/**
 * File: FilePropertiesMD.h
 * License: Part of the MetaCG project. Licensed under BSD 3 clause license. See LICENSE.txt file at
 * https://github.com/tudasc/metacg/LICENSE.txt
 */

#ifndef METACG_FILEPROPERTIESMD_H
#define METACG_FILEPROPERTIESMD_H

#include "metadata/MetaData.h"

namespace metacg {

class FilePropertiesMD : public metacg::MetaData::Registrar<FilePropertiesMD> {
 public:
  static constexpr const char* key = "fileProperties";
  FilePropertiesMD() : fromSystemInclude(false) {}
  explicit FilePropertiesMD(const nlohmann::json& j, StrToNodeMapping&) {
    if (j.is_null()) {
      metacg::MCGLogger::instance().getConsole()->error("Could not retrieve meta data for {}", key);
      return;
    }
    fromSystemInclude = j["systemInclude"];
  }

 private:
  FilePropertiesMD(const FilePropertiesMD& other) : fromSystemInclude(other.fromSystemInclude) {}

 public:
  nlohmann::json to_json(NodeToStrMapping& nodeToStr) const final {
    nlohmann::json j;
    j["systemInclude"] = fromSystemInclude;
    return j;
  }

  virtual const char* getKey() const final { return key; }

  void merge(const MetaData& toMerge) final {
    assert(toMerge.getKey() == getKey() && "Trying to merge FilePropertiesMD with metadata of different types");

    const FilePropertiesMD* toMergeDerived = static_cast<const FilePropertiesMD*>(&toMerge);

    this->fromSystemInclude |= toMergeDerived->fromSystemInclude;
  }

  std::unique_ptr<MetaData> clone() const final {
    return std::unique_ptr<FilePropertiesMD>(new FilePropertiesMD(*this));
  }

  bool fromSystemInclude;
};

}

#endif  // METACG_FILEPROPERTIESMD_H

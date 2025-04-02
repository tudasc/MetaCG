/**
 * File: FileMD.h
 * License: Part of the MetaCG project. Licensed under BSD 3 clause license. See LICENSE.txt file at
 * https://github.com/tudasc/metacg/LICENSE.txt
 */

#ifndef METACG_FILEMD_H
#define METACG_FILEMD_H

#include "metadata/MetaData.h"

class FilePropertiesMD : public metacg::MetaData::Registrar<FilePropertiesMD> {
 public:
  static constexpr const char* key = "fileProperties";
  FilePropertiesMD() : fromSystemInclude(false) {}
  explicit FilePropertiesMD(const nlohmann::json& j) {
    if (j.is_null()) {
      metacg::MCGLogger::instance().getConsole()->error("Could not retrieve meta data for {}", key);
      return;
    }
    fromSystemInclude = j["systemInclude"];
  }

 private:
  FilePropertiesMD(const FilePropertiesMD& other)
      : fromSystemInclude(other.fromSystemInclude) {}

 public:
  nlohmann::json to_json() const final {
    nlohmann::json j;
    j["systemInclude"] = fromSystemInclude;
    return j;
  };

  virtual const char* getKey() const final { return key; }

  void merge(const MetaData& toMerge) final {
    if (std::strcmp(toMerge.getKey(), getKey()) != 0) {
      metacg::MCGLogger::instance().getErrConsole()->error(
          "The MetaData which was tried to merge with FilePropertiesMD was of a different MetaData type");
      abort();
    }
    const FilePropertiesMD* toMergeDerived = static_cast<const FilePropertiesMD*>(&toMerge);

    this->fromSystemInclude |= toMergeDerived->fromSystemInclude;
  }

  MetaData* clone() const final { return new FilePropertiesMD(*this); }

  bool fromSystemInclude;
};

#endif  // METACG_FILEMD_H

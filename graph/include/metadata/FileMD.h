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
  static constexpr const char *key = "fileProperties";
  FilePropertiesMD() : origin("INVALID"), fromSystemInclude(false), lineNumber(0) {}
  explicit FilePropertiesMD(const nlohmann::json &j) {
    if (j.is_null()) {
      metacg::MCGLogger::instance().getConsole()->error("Could not retrieve meta data for {}", key);
      return;
    }

    std::string fileOrigin = j["origin"].get<std::string>();
    bool isFromSystemInclude = j["systemInclude"].get<bool>();
    origin = fileOrigin;
    fromSystemInclude = isFromSystemInclude;
  }

 private:
  FilePropertiesMD(const FilePropertiesMD& other) : origin(other.origin), fromSystemInclude(other.fromSystemInclude),
                                                                lineNumber(other.lineNumber) {}

 public:
  nlohmann::json to_json() const final {
    nlohmann::json j;
    j["origin"] = origin;
    j["systemInclude"] = fromSystemInclude;
    return j;
  };

  virtual const char *getKey() const final { return key; }

  void merge(const MetaData& toMerge) final {
    if (std::strcmp(toMerge.getKey(), getKey()) != 0) {
      metacg::MCGLogger::instance().getErrConsole()->error(
          "The MetaData which was tried to merge with FilePropertiesMD was of a different MetaData type");
      abort();
    }
    const FilePropertiesMD* toMergeDerived = static_cast<const FilePropertiesMD*>(&toMerge);
    if (this->origin.empty()) {
      this->origin = toMergeDerived->origin;
      this->fromSystemInclude = toMergeDerived->fromSystemInclude;
    }
    // TODO: Save all paths, if multiple definitions? For now, just keep the current data.
  }

  MetaData* clone() const final {
    return new FilePropertiesMD(*this);
  }

  std::string origin;
  bool fromSystemInclude;
  int lineNumber;
};

#endif  // METACG_FILEMD_H

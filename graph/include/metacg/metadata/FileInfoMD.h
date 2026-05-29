/**
 * File: FileInfoMD.h
 * License: Part of the MetaCG project. Licensed under BSD 3 clause license. See LICENSE.txt file at
 * https://github.com/tudasc/metacg/LICENSE.txt
 */

#ifndef CGCOLLECTOR2_FILEINFOMETADATA_H
#define CGCOLLECTOR2_FILEINFOMETADATA_H

#include "metacg/metadata/MetaData.h"

/**
 * This is the same metadata that is used inside the MetaCG library
 * The same restrictions apply:
 * Implement a static key, and the three virtual functions, and register your metadata via the Registrar
 */

class FileInfoMetadata : public metacg::MetaData::Registrar<FileInfoMetadata> {
 public:
  static constexpr const char* key = "FilePropertiesMetaData";
  FileInfoMetadata() : origin("INVALID"), fromSystemInclude(false), lineNumber(0) {}
  explicit FileInfoMetadata(const nlohmann::json& j, metacg::StrToNodeMapping& strToNode) {
    if (j.is_null()) {
      metacg::MCGLogger::instance().getConsole()->trace("Could not retrieve meta data for fileProperties");
      return;
    }
    origin = j["origin"].get<std::string>();
    fromSystemInclude = j["systemInclude"].get<bool>();
  }

  FileInfoMetadata(const FileInfoMetadata& other)
      : origin(other.origin), fromSystemInclude(other.fromSystemInclude), lineNumber(other.lineNumber) {}

  nlohmann::json toJson(metacg::NodeToStrMapping&) const final {
    nlohmann::json j;
    j["origin"] = origin;
    j["systemInclude"] = fromSystemInclude;
    return j;
  }

  void applyMapping(const metacg::GraphMapping&) final {}

  void merge(const MetaData&, std::optional<metacg::MergeAction>, const metacg::GraphMapping&) final {}
  const char* getKey() const final { return key; }

  std::unique_ptr<MetaData> clone() const final { return std::make_unique<FileInfoMetadata>(*this); }

  std::string origin;
  bool fromSystemInclude;
  int lineNumber;
};
#endif  // CGCOLLECTOR2_FILEINFOMETADATA_H

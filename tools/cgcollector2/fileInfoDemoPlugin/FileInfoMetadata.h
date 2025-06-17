/**
* File: FileInfoMetadata.h
* License: Part of the MetaCG project. Licensed under BSD 3 clause license. See LICENSE.txt file at
* https://github.com/tudasc/metacg/LICENSE.txt
*/

#ifndef CGCOLLECTOR2_FILEINFOMETADATA_H
#define CGCOLLECTOR2_FILEINFOMETADATA_H

#include "metadata/MetaData.h"

/**
 * This is the same metadata that is used inside the MetaCG library
 * The same restrictions apply:
 * Implement a static key, and the three virtual functions, and register your metadata via the Registrar
 */

class FileInfoMetadata : public metacg::MetaData::Registrar<FileInfoMetadata> {
 public:
  static constexpr const char* key = "FilePropertiesMetaData";
  FileInfoMetadata() : origin("INVALID"), fromSystemInclude(false), lineNumber(0) {}
  explicit FileInfoMetadata(const nlohmann::json& j);

 private:
  FileInfoMetadata(const FileInfoMetadata& other)
      : origin(other.origin), fromSystemInclude(other.fromSystemInclude), lineNumber(other.lineNumber) {}

 public:
  nlohmann::json to_json() const final;

  virtual const char* getKey() const final { return key; }

  void merge(const MetaData& toMerge) final;

  MetaData* clone() const final { return new FileInfoMetadata(*this); }

  std::string origin;
  bool fromSystemInclude;
  int lineNumber;
};
#endif  // CGCOLLECTOR2_FILEINFOMETADATA_H

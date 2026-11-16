/**
 * File: FileInfoMD.h
 * License: Part of the MetaCG project. Licensed under BSD 3 clause license. See LICENSE.txt file at
 * https://github.com/tudasc/metacg/LICENSE.txt
 */

#ifndef METACG_FILEINFOMD_H
#define METACG_FILEINFOMD_H

#include "metacg/metadata/MetaData.h"

namespace metacg {

/**
 * File related information of a function.
 *
 * The origin is a first-class node property since format version 4. The origin stored here is only used by collectors
 * that can not set the node origin directly. It is optional in the serialized form and omitted when empty.
 */
class FileInfoMD : public metacg::MetaData::Registrar<FileInfoMD> {
 public:
  static constexpr const char* key = "fileProperties";
  FileInfoMD() : fromSystemInclude(false), lineNumber(0) {}
  explicit FileInfoMD(const nlohmann::json& j, StrToNodeMapping&) : fromSystemInclude(false), lineNumber(0) {
    if (j.is_null()) {
      metacg::MCGLogger::logErrorUnique("Could not retrieve meta data for {}", key);
      return;
    }
    if (auto it = j.find("origin"); it != j.end() && it->is_string()) {
      origin = it->get<std::string>();
    }else {
      MCGLogger::logWarnUnique("Found fileProperties metadata without origin field");
    }
    fromSystemInclude = j["systemInclude"].get<bool>();
    if (auto it = j.find("lineNumber"); it != j.end() && it->is_string()) {
      lineNumber = j["lineNumber"].get<int>();
    }else {
      MCGLogger::logWarnUnique("Found fileProperties metadata without lineNumber field");
    }
  }

 private:
  FileInfoMD(const FileInfoMD& other)
      : origin(other.origin), fromSystemInclude(other.fromSystemInclude), lineNumber(other.lineNumber) {}

 public:
  nlohmann::json toJson(NodeToStrMapping&) const final {
    nlohmann::json j;
    j["origin"] = origin;
    j["systemInclude"] = fromSystemInclude;
    j["lineNumber"] = lineNumber;
    return j;
  }

  const char* getKey() const final { return key; }

  void merge(const MetaData& toMerge, std::optional<MergeAction> action, const GraphMapping&) final {
    assert(toMerge.getKey() == getKey() && "Trying to merge FileInfoMD with metadata of different types");

    // Adopt file info from the other node, if it's replacing this one
    if (action && action->replace) {
      const FileInfoMD* toMergeDerived = static_cast<const FileInfoMD*>(&toMerge);
      this->origin = toMergeDerived->origin;
      this->fromSystemInclude = toMergeDerived->fromSystemInclude;
      this->lineNumber = toMergeDerived->lineNumber;
    }
  }

  std::unique_ptr<MetaData> clone() const final { return std::unique_ptr<FileInfoMD>(new FileInfoMD(*this)); }

  void applyMapping(const GraphMapping&) override {}

  std::string origin;
  bool fromSystemInclude;
  int lineNumber;
};

}  // namespace metacg

#endif  // METACG_FILEINFOMD_H

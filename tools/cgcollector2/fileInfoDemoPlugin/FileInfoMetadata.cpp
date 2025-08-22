/**
* File: FileInfoMetadata.cpp
* License: Part of the MetaCG project. Licensed under BSD 3 clause license. See LICENSE.txt file at
* https://github.com/tudasc/metacg/LICENSE.txt
*/

#include "FileInfoMetadata.h"

FileInfoMetadata::FileInfoMetadata(const nlohmann::json& j, metacg::StrToNodeMapping&) {
  if (j.is_null()) {
    metacg::MCGLogger::instance().getConsole()->trace("Could not retrieve meta data for fileProperties");
    return;
  }
  origin = j["origin"].get<std::string>();
  fromSystemInclude = j["systemInclude"].get<bool>();
}

nlohmann::json FileInfoMetadata::toJson(metacg::NodeToStrMapping&) const {
  nlohmann::json j;
  j["origin"] = origin;
  j["systemInclude"] = fromSystemInclude;
  return j;
}
void FileInfoMetadata::merge(const MetaData&, std::optional<metacg::MergeAction>, const metacg::GraphMapping&) {
  // Not implemented yet
}

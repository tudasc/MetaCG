/**
 * File: VersionFourMCGWriter.h
 * License: Part of the MetaCG project. Licensed under BSD 3 clause license. See LICENSE.txt file at
 * https://github.com/tudasc/metacg/LICENSE.txt
 */

#ifndef METACG_VERSIONFOURMCGWRITER_H
#define METACG_VERSIONFOURMCGWRITER_H

#include "MCGWriter.h"
#include "config.h"
namespace metacg::io {

class VersionFourMCGWriter : public MCGWriter {
 public:
  // Fixme: Make the MetaCG versions, not CGCollector versions
  explicit VersionFourMCGWriter(
      MCGFileInfo fileInfo = {{4, 0}, {"MetaCG", MetaCG_VERSION_MAJOR, MetaCG_VERSION_MINOR, MetaCG_GIT_SHA}},
      bool useNamesAsIds = false, bool exportSorted = false)
      : MCGWriter(std::move(fileInfo)), useNamesAsIds(useNamesAsIds), exportSorted(exportSorted) {}

  void write(const Callgraph* graph, JsonSink& js) override;

  static void sortCallgraph(nlohmann::json& j);

  void setUseNamesAsIds(bool useNames) {
    this->useNamesAsIds = useNames;
  }

  void setExportSorted(bool sort) {
    this->exportSorted = sort;
  }

 private:
  bool useNamesAsIds;
  bool exportSorted;
};

}  // namespace metacg::io

#endif  // METACG_VERSIONFOURMCGWRITER_H

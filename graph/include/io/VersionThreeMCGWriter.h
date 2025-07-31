/**
 * File: VersionThreeMCGWriter.h
 * License: Part of the MetaCG project. Licensed under BSD 3 clause license. See LICENSE.txt file at
 * https://github.com/tudasc/metacg/LICENSE.txt
 */

#ifndef METACG_VERSIONTHREEMCGWRITER_H
#define METACG_VERSIONTHREEMCGWRITER_H

#include "MCGWriter.h"
#include "config.h"
namespace metacg::io {

class VersionThreeMCGWriter : public MCGWriter {
 public:
  // Fixme: Make the MetaCG versions, not CGCollector versions
  explicit VersionThreeMCGWriter(
      MCGFileInfo fileInfo = {{3, 0}, {"MetaCG", MetaCG_VERSION_MAJOR, MetaCG_VERSION_MINOR, MetaCG_GIT_SHA}},
      bool debug = false, bool exportSorted = false)
      : MCGWriter(std::move(fileInfo)), outputDebug(debug), exportSorted(exportSorted) {}

  void write(const Callgraph* graph, JsonSink& js) override;

  static void convertToDebug(nlohmann::json& json);

  static void sortCallgraph(nlohmann::json& j);

 private:
  bool outputDebug;
  bool exportSorted;
};

}  // namespace metacg::io

#endif  // METACG_VERSIONTHREEMCGWRITER_H

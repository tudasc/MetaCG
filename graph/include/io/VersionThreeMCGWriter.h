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
      graph::MCGManager &mcgm,
      MCGFileInfo fileInfo = {{3, 0}, {"MetaCG", CGCollector_VERSION_MAJOR, CGCollector_VERSION_MINOR, MetaCG_GIT_SHA}},
      bool debug = false, bool exportSorted=false)
      : MCGWriter(mcgm, std::move(fileInfo)), outputDebug(debug), exportSorted(exportSorted) {}

  void write(metacg::io::JsonSink &js) override;

 private:
  bool outputDebug;
  bool exportSorted;
  void convertToDebug(nlohmann::json &json);
  void sortCallgraph(nlohmann::json &j) const;
};

}  // namespace metacg::io

#endif  // METACG_VERSIONTHREEMCGWRITER_H

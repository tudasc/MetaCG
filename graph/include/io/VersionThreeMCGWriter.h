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
      MCGFileInfo fileInfo = {{3, 0}, {"MetaCG", CGCollector_VERSION_MAJOR, CGCollector_VERSION_MINOR, MetaCG_GIT_SHA}},
      bool debug = false, bool exportSorted=false)
      : MCGWriter(std::move(fileInfo)), outputDebug(debug), exportSorted(exportSorted) {}


  /**
   *
   * Writes the current active graph from the Callgraph-manager to a specified JsonSink
   *
   * @param js which sink to write to
   */
  void write(metacg::io::JsonSink &js);

  /**
   *
   * Writes a specified callgraph to a specified JsonSink
   *
   * @param graph which graph to write out
   * @param js which sink to write to
   */
  void write(metacg::Callgraph* cg, metacg::io::JsonSink &js) override;

  /**
   *
   * Retrieves the callgraph with the given name from the Callgraph Manager
   * and writes to a specified JsonSink
   *
   * @param CGName the name of the graph to write out
   * @param js which sink to write to
   */
  void write(const std::string& CGName, metacg::io::JsonSink &js);

 private:
  bool outputDebug;
  bool exportSorted;
  void convertToDebug(nlohmann::json &json);
  void sortCallgraph(nlohmann::json &j) const;
};

}  // namespace metacg::io

#endif  // METACG_VERSIONTHREEMCGWRITER_H

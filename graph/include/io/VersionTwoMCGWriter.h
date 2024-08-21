/**
 * File: VersionTwoMCGWriter.h
 * License: Part of the MetaCG project. Licensed under BSD 3 clause license. See LICENSE.txt file at
 * https://github.com/tudasc/metacg/LICENSE.txt
 */

#ifndef METACG_VERSIONTWOMCGWRITER_H
#define METACG_VERSIONTWOMCGWRITER_H

#include "MCGWriter.h"

namespace metacg::io {

class VersionTwoMCGWriter : public MCGWriter {
 public:
  explicit VersionTwoMCGWriter(metacg::MCGFileInfo fileInfo = metacg::getVersionTwoFileInfo(metacg::getCGCollectorGeneratorInfo()))
      : MCGWriter(std::move(fileInfo)) {}
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
  static void downgradeV3FormatToV2Format(nlohmann::json &cg);
};
}  // namespace metacg::io

#endif  // METACG_VERSIONTWOMCGWRITER_H

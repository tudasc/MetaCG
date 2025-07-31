/**
 * File: MCGWriter.h
 * License: Part of the MetaCG project. Licensed under BSD 3 clause license. See LICENSE.txt file at
 * https://github.com/tudasc/metacg/LICENSE.txt
 */

#ifndef METACG_MCGWRITER_H
#define METACG_MCGWRITER_H

#include "Callgraph.h"
#include "MCGBaseInfo.h"

#include "nlohmann/json.hpp"

#include <fstream>

namespace metacg::io {

/**
 * Target sink to serialize the CG into a json object.
 */
class JsonSink {
 public:
  void setJson(nlohmann::json jsonIn) { j = jsonIn; }

  [[nodiscard]] const nlohmann::json& getJson() const { return j; }

  /**
   * Outputs the Json stored in this sink into os and flushes.
   * @param os
   */
  void output(std::ostream& os) {
    os << j;
    os.flush();
  }

 private:
  nlohmann::json j{};
};

/**
 * Class to serialize the CG.
 * This class is intended to be subclassed for every file format version
 */
class MCGWriter {
 public:
  explicit MCGWriter(MCGFileInfo fileInfo) : fileInfo(std::move(fileInfo)) {}
  /**
   *
   * Writes a specified callgraph to a specified JsonSink
   *
   * @param graph which graph to write out
   * @param js which sink to write to
   */
  virtual void write(const Callgraph* graph, JsonSink& js) = 0;

  /**
   * Writes the (managed) call graph with the given name to a json sink.
   * Note: Distinct name to avoid overload resolution issues.
   * @param cgName Name of the graph
   * @param js The json sink to write to
   */
  void writeNamedGraph(const std::string& cgName, JsonSink& js);

  /**
   * Writes the active call graph to a json sink.
   * Note: Distinct name to avoid overload resolution issues.
   * @param js The json sink to write to
   */
  void writeActiveGraph(JsonSink& js);

  virtual ~MCGWriter() = default;

 protected:
  /**
   * Adds the CG version data to the MetaCG in json Format.
   * @param j
   */
  void attachMCGFormatHeader(nlohmann::json& j) {
    const auto formatInfo = fileInfo.formatInfo;
    const auto generatorInfo = fileInfo.generatorInfo;
    j = {{formatInfo.metaInfoFieldName, {}}, {formatInfo.cgFieldName, {}}};
    j[formatInfo.metaInfoFieldName] = {{formatInfo.version.getJsonIdentifier(), formatInfo.version.getVersionStr()},
                                       {generatorInfo.getJsonIdentifier(),
                                        {{generatorInfo.getJsonNameIdentifier(), generatorInfo.name},
                                         {generatorInfo.getJsonVersionIdentifier(), generatorInfo.getVersionStr()},
                                         {generatorInfo.getJsonShaIdentifier(), generatorInfo.sha}}}};
  }
  MCGFileInfo fileInfo;
};

/**
 * Factory function to instantiate the correct writer implementation for the given format.
 * @param version The format version.
 * @return A unique pointer to the instantiated writer. Empty, if there is no writer matching the format version.
 */
std::unique_ptr<MCGWriter> createWriter(int version);

}  // namespace metacg::io

#endif  // METACG_MCGWRITER_H

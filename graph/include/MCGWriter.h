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

  [[nodiscard]] const nlohmann::json &getJson() const { return j; }

  /**
   * Outputs the Json stored in this sink into os and flushes.
   * @param os
   */
  void output(std::ostream &os) {
    os << j;
    os.flush();
  }

 private:
  nlohmann::json j{};
};

/**
 * Class to serialize the CG.
 */
class MCGWriter {
 public:
  explicit MCGWriter(graph::MCGManager &mcgm,
                     MCGFileInfo fileInfo = getVersionTwoFileInfo(getCGCollectorGeneratorInfo()))
      : mcgManager(mcgm), fileInfo(std::move(fileInfo)) {}

  void write(JsonSink &js);

 private:
  /**
   * Adds the CG version data to the MetaCG in json Format.
   * @param j
   */
  inline void attachMCGFormatHeader(nlohmann::json &j) {
    const auto formatInfo = fileInfo.formatInfo;
    const auto generatorInfo = fileInfo.generatorInfo;
    j = {{formatInfo.metaInfoFieldName, {}}, {formatInfo.cgFieldName, {}}};
    j[formatInfo.metaInfoFieldName] = {{formatInfo.version.getJsonIdentifier(), formatInfo.version.getVersionStr()},
                                       {generatorInfo.getJsonIdentifier(),
                                        {{generatorInfo.getJsonNameIdentifier(), generatorInfo.name},
                                         {generatorInfo.getJsonVersionIdentifier(), generatorInfo.getVersionStr()},
                                         {generatorInfo.getJsonShaIdentifier(), generatorInfo.sha}}}};
  }

  /**
   * General construction of node data, e.g., function name.
   * @param node
   */
  void createNodeData(const CgNode *node, nlohmann::json &j) const;

  graph::MCGManager &mcgManager;
  MCGFileInfo fileInfo;
};

}  // namespace metacg::io

#endif  // METACG_MCGWRITER_H

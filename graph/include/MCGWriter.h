/**
 * File: MCGWriter.h
 * License: Part of the MetaCG project. Licensed under BSD 3 clause license. See LICENSE.txt file at
 * https://github.com/tudasc/metacg/LICENSE.txt
 */

#ifndef METACG_MCGWRITER_H
#define METACG_MCGWRITER_H

#include "Callgraph.h"
#include "MCGBaseInfo.h"
#include "config.h"

#include "nlohmann/json.hpp"

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
  void createNodeData(const CgNodePtr node, nlohmann::json &j);

  /**
   * Using the stored MetaDataHandler in MCGM to walk MCG and extract all meta data.
   * @param node
   * @param mcgm
   */
  void createAndAddMetaData(CgNodePtr node, const graph::MCGManager &mcgm, nlohmann::json &j);

  graph::MCGManager &mcgManager;
  MCGFileInfo fileInfo;
};

/*
 * Old Annotation mechanism.
 * TODO: Replace fully with MCGWriter mechanism.
 */
template <typename PropRetriever>
int doAnnotate(metacg::Callgraph &cg, PropRetriever retriever, nlohmann::json &j) {
  const auto functionElement = [](nlohmann::json &container, auto name) {
    for (nlohmann::json::iterator it = container.begin(); it != container.end(); ++it) {
      if (it.key() == name) {
        return it;
      }
    }
    return container.end();
  };

  const auto holdsValue = [](auto jsonIt, auto jsonEnd) { return jsonIt != jsonEnd; };

  int annots = 0;
  for (const auto &node : cg) {
    if (retriever.handles(node)) {
      auto funcElem = functionElement(j, node->getFunctionName());

      if (holdsValue(funcElem, j.end())) {
        auto &nodeMeta = (*funcElem)["meta"];
        nodeMeta[retriever.toolName()] = retriever.value(node);
        annots++;
      }
    }
  }
  return annots;
}

template <typename PropRetriever>
void annotateJSON(metacg::Callgraph &cg, const std::string &filename, PropRetriever retriever) {
  nlohmann::json j;
  {
    std::ifstream in(filename);
    in >> j;
  }

  int annotated = metacg::io::doAnnotate(cg, retriever, j);
  spdlog::get("console")->trace("Annotated {} json nodes", annotated);

  {
    std::ofstream out(filename);
    out << j << std::endl;
  }
}

}  // namespace metacg::io
#endif  // METACG_MCGWRITER_H

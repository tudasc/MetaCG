/**
 * File: MCGWriter.cpp
 * License: Part of the MetaCG project. Licensed under BSD 3 clause license. See LICENSE.txt file at
 * https://github.com/tudasc/metacg/LICENSE.txt
 */

#include "MCGWriter.h"
#include "MCGManager.h"
#include "Util.h"

using FunctionNames = std::set<std::string>;

metacg::MCGFileInfo metacg::getVersionTwoFileInfo(metacg::MCGGeneratorVersionInfo mcgGenInfo) {
  return {MCGFileFormatInfo(2, 0), mcgGenInfo};
}

metacg::MCGGeneratorVersionInfo metacg::getCGCollectorGeneratorInfo() {
  std::string cgcCollName("CGCollector");
  return {cgcCollName, CGCollector_VERSION_MAJOR, CGCollector_VERSION_MINOR, MetaCG_GIT_SHA};
}

void metacg::io::MCGWriter::write(JsonSink &js) {
  nlohmann::json j;
  attachMCGFormatHeader(j);

  for (const auto &n : *mcgManager.getCallgraph()) {
    createNodeData(n, j);  // general node data?
    createAndAddMetaData(n, mcgManager, j);
  }

  js.setJson(j);
}

void metacg::io::MCGWriter::createNodeData(const CgNodePtr node, nlohmann::json &j) const {
  auto console = metacg::MCGLogger::instance().getConsole();
  console->trace("Creating Node data for {}", node->getFunctionName());
  // Currently correctly stored in CgNode
  const auto funcName = node->getFunctionName();
  const auto isVirtual = node->isVirtual();
  const auto callees = node->getChildNodes();
  const auto callers = node->getParentNodes();
  const auto hasBody = node->getHasBody();

  // XXX Currently, this information is lost when building the in-memory metacg
  //  const auto overrides = node->getOverrides();
  //  const auto overriddenBy = node->getOverriddenBy();
  //  const auto doesOverride = node->getDoesOverride();
  const bool doesOverride{false};
  const std::set<std::string> overrides;
  const std::set<std::string> overriddenBy;
  //  insertNode(j, funcName, callees, callers, overriddenBy, overrides, isVirtual, doesOverride, hasBody, 2);

  const auto nodeInfo = fileInfo.nodeInfo;
  j[fileInfo.formatInfo.cgFieldName][funcName] = {{nodeInfo.calleesStr, metacg::util::to_string(callees)},
                                                  {nodeInfo.isVirtualStr, isVirtual},
                                                  {nodeInfo.doesOverrideStr, doesOverride},
                                                  {nodeInfo.overridesStr, overrides},
                                                  {nodeInfo.overriddenByStr, overriddenBy},
                                                  {nodeInfo.callersStr, metacg::util::to_string(callers)},
                                                  {nodeInfo.hasBodyStr, hasBody},
                                                  {nodeInfo.metaStr, nullptr}};
}

void metacg::io::MCGWriter::createAndAddMetaData(CgNodePtr node, const metacg::graph::MCGManager &mcgm,
                                                 nlohmann::json &j) {
  const auto funcName = node->getFunctionName();
  const auto mdHandlers = mcgm.getMetaHandlers();
  for (const auto mdh : mdHandlers) {
    if (!mdh->handles(node)) {
      continue;
    }
    const auto mdJson = mdh->value(node);
    j[fileInfo.formatInfo.cgFieldName][funcName][fileInfo.nodeInfo.metaStr][mdh->toolName()] = mdJson;
  }
}

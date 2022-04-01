/**
 * File: MCGWriter.cpp
 * License: Part of the MetaCG project. Licensed under BSD 3 clause license. See LICENSE.txt file at
 * https://github.com/tudasc/metacg/LICENSE.txt
 */

#include "MCGWriter.h"
#include "MCGManager.h"
#include "Util.h"

using FunctionNames = std::set<std::string>;

//inline void insertNode(nlohmann::json &callgraph, const std::string &nodeName, const FunctionNames &callees,
//                       const FunctionNames &callers, const FunctionNames &overriddenBy,
//                       const FunctionNames &overriddenFunctions, const bool isVirtual, const bool doesOverride,
//                       const bool hasBody, int version) {
//  if (version == 1) {
//    callgraph[nodeName] = {{"callees", callees},
//                           {"isVirtual", isVirtual},
//                           {"doesOverride", doesOverride},
//                           {"overriddenFunctions", overriddenFunctions},
//                           {"overriddenBy", overriddenBy},
//                           {"parents", callers},
//                           {"hasBody", hasBody}};
//  } else if (version == 2) {
//    callgraph["_CG"][nodeName] = {{"callees", callees},
//                                  {"isVirtual", isVirtual},
//                                  {"doesOverride", doesOverride},
//                                  {"overrides", overriddenFunctions},
//                                  {"overriddenBy", overriddenBy},
//                                  {"callers", callers},
//                                  {"hasBody", hasBody}};
//  }
//}

void metacg::io::MCGWriter::write(JsonSink &js) {
  nlohmann::json j;
  attachFormatTwoHeader(j);

  for (const auto &n : *mcgManager.getCallgraph()) {
    createNodeData(n, j);  // general node data?
    createAndAddMetaData(n, mcgManager, j);
  }

  js.setJson(j);
}

void metacg::io::MCGWriter::createNodeData(const CgNodePtr node, nlohmann::json &j) {
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
  const bool doesOverride {false};
  const std::set<std::string> overrides;
  const std::set<std::string> overriddenBy;
//  insertNode(j, funcName, callees, callers, overriddenBy, overrides, isVirtual, doesOverride, hasBody, 2);

  j["_CG"][funcName] = {{"callees", metacg::util::to_string(callees)},
                        {"isVirtual", isVirtual},
                        {"doesOverride", doesOverride},
                        {"overrides", overrides},
                        {"overriddenBy", overriddenBy},
                        {"callers", metacg::util::to_string(callers)},
                        {"hasBody", hasBody},
                        {"meta", nullptr}};
}

void metacg::io::MCGWriter::createAndAddMetaData(CgNodePtr node, const metacg::graph::MCGManager &mcgm, nlohmann::json &j) {
  const auto funcName = node->getFunctionName();
  const auto mdHandlers = mcgm.getMetaHandlers();
  for (const auto mdh : mdHandlers) {
    if (!mdh->handles(node)) {
      continue;
    }
    const auto mdJson = mdh->value(node);
    j["_CG"][funcName]["meta"][mdh->toolName()] = mdJson;
  }
}

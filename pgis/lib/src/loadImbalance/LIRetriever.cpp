/**
 * File: LIRetriever.cpp
 * License: Part of the MetaCG project. Licensed under BSD 3 clause license. See LICENSE.txt file at https://github.com/tudasc/metacg/LICENSE.txt
 */
#include "CallgraphManager.h"
#include "loadImbalance/LIRetriever.h"

using namespace LoadImbalance;

bool LIRetriever::handles(const CgNodePtr n) { return n->has<LoadImbalance::LIMetaData>(); }

LoadImbalance::LIMetaData LIRetriever::value(const CgNodePtr n) { return *(n->get<LoadImbalance::LIMetaData>()); }

std::string LIRetriever::toolName() { return "LIData"; }


void LoadImbalanceMetaDataHandler::read(const nlohmann::json &j, const std::string &functionName) {
  auto jIt = j[toolname];
  if (jIt.is_null()) {
    spdlog::get("console")->warn("Could not read LoadImbalance info");
  }

  bool irrelevant = jIt["irrelevant"].get<bool>();
  bool visited = jIt["visited"].get<bool>();

  auto node = cgm->findOrCreateNode(functionName);
  auto md = new LoadImbalance::LIMetaData();
  if (irrelevant) {
    spdlog::get("console")->debug("Setting flag irrelevant");
    md->flag(LoadImbalance::FlagType::Irrelevant);
  }
  if (visited) {
    spdlog::get("console")->debug("Setting flag visited");
    md->flag(LoadImbalance::FlagType::Visited);
  }
  node->addMetaData(md);
}

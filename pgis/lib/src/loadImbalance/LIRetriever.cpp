/**
 * File: LIRetriever.cpp
 * License: Part of the metacg project. Licensed under BSD 3 clause license. See LICENSE.txt file at
 * https://github.com/tudasc/metacg/LICENSE.txt
 */
#include "loadImbalance/LIRetriever.h"
#include "MCGManager.h"

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

  auto node = mcgm->findOrCreateNode(functionName);
  // JP: Nodes should always have LIMetaData attached (added at construction-time)
  if (node->has<LoadImbalance::LIMetaData>()) {
    auto md = node->get<LoadImbalance::LIMetaData>();
    if (irrelevant) {
      spdlog::get("console")->debug("Setting flag irrelevant");
      md->flag(LoadImbalance::FlagType::Irrelevant);
    }
    if (visited) {
      spdlog::get("console")->debug("Setting flag visited");
      md->flag(LoadImbalance::FlagType::Visited);
    }
    md->setVirtual(node->isVirtual());  // TODO Unify
  } else {
    spdlog::get("errconsole")->warn("Did not find a load-imbalance metadata. This is fishy!");
    auto md = new LoadImbalance::LIMetaData();
    if (irrelevant) {
      spdlog::get("console")->debug("Setting flag irrelevant");
      md->flag(LoadImbalance::FlagType::Irrelevant);
    }
    if (visited) {
      spdlog::get("console")->debug("Setting flag visited");
      md->flag(LoadImbalance::FlagType::Visited);
    }
    md->setVirtual(node->isVirtual());  // TODO Unify
    // XXX When no metadata was attached, this should not assert.
    node->addMetaData(md);
  }
}

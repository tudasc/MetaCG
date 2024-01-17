/**
 * File: LegacyMCGReader.cpp
 * License: Part of the metacg project. Licensed under BSD 3 clause license. See LICENSE.txt file at
 * https://github.com/tudasc/metacg/LICENSE.txt
 */

#include "LegacyMCGReader.h"
#include "CgNode.h"
#include "Timing.h"
#include "Util.h"

#include <loadImbalance/LIMetaData.h>

namespace metacg::io {
/**
 * Version one Reader
 */
void VersionOneMetaCGReader::read(metacg::graph::MCGManager &cgManager) {
  metacg::RuntimeTimer rtt("Version One Reader");
  auto console = metacg::MCGLogger::instance().getConsole();

  console->trace("Reading");
  auto j = source.get();

  for (json::iterator it = j.begin(); it != j.end(); ++it) {
    console->trace("Inserting node for key {}", it.key());
    auto &fi = getOrInsert(it.key());

    /* This is structural and basic information */
    fi.functionName = it.key();
    setIfNotNull(fi.hasBody, it, "hasBody");
    setIfNotNull(fi.isVirtual, it, "isVirtual");
    setIfNotNull(fi.doesOverride, it, "doesOverride");

    std::set<std::string> callees;
    setIfNotNull(callees, it, "callees");
    fi.callees.insert(callees.begin(), callees.end());

    std::set<std::string> ofs;
    setIfNotNull(ofs, it, "overriddenFunctions");
    fi.overriddenFunctions.insert(ofs.begin(), ofs.end());

    std::set<std::string> overriddenBy;
    setIfNotNull(overriddenBy, it, "overriddenBy");
    fi.overriddenBy.insert(overriddenBy.begin(), overriddenBy.end());

    std::set<std::string> ps;
    setIfNotNull(ps, it, "parents");
    fi.parents.insert(ps.begin(), ps.end());

    /* Meta information, will be refactored any way */
    setIfNotNull(fi.numStatements, it, "numStatements");

    // this needs to be done in a more generic way in the future!
    if (!it.value()["meta"].is_null() && !it.value()["meta"]["LIData"].is_null()) {
      fi.visited = it.value()["meta"]["LIData"].value("visited", false);
      fi.irrelevant = it.value()["meta"]["LIData"].value("irrelevant", false);
    }
  }

  auto potentialTargets = buildVirtualFunctionHierarchy(cgManager);
  buildGraph(cgManager, potentialTargets);
  addNumStmts(cgManager);

  // set load imbalance flags in CgNode
  for (const auto &pfi : functions) {
    std::optional<metacg::CgNode *> opt_f = cgManager.getCallgraph()->getNode(pfi.first);
    if (opt_f.has_value()) {
      metacg::CgNode *node = opt_f.value();
      node->getOrCreateMD<LoadImbalance::LIMetaData>();
      node->getOrCreateMD<LoadImbalance::LIMetaData>()->setVirtual(pfi.second.isVirtual);

      if (pfi.second.visited) {
        node->getOrCreateMD<LoadImbalance::LIMetaData>()->flag(LoadImbalance::FlagType::Visited);
      }

      if (pfi.second.irrelevant) {
        node->getOrCreateMD<LoadImbalance::LIMetaData>()->flag(LoadImbalance::FlagType::Irrelevant);
      }
    }
  }
}

void VersionOneMetaCGReader::addNumStmts(metacg::graph::MCGManager &cgm) {
  for (const auto &[k, fi] : functions) {
    auto g = cgm.getCallgraph();
    auto node = g->getNode(fi.functionName);
    node->getOrCreateMD<pira::PiraOneData>();
    node->getOrCreateMD<pira::BaseProfileData>();
    assert(node != nullptr && "Nodes with #statements attached should be available");
    if (node->has<pira::PiraOneData>()) {
      auto pod = node->get<pira::PiraOneData>();
      pod->setNumberOfStatements(fi.numStatements);
      pod->setHasBody(fi.hasBody);
    } else {
      assert(false && "pira::PiraOneData metadata should be default-constructed atm.");
    }
  }
}

}  // namespace metacg::io

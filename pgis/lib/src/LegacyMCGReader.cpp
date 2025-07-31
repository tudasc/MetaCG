/**
 * File: LegacyMCGReader.cpp
 * License: Part of the metacg project. Licensed under BSD 3 clause license. See LICENSE.txt file at
 * https://github.com/tudasc/metacg/LICENSE.txt
 */

#include "LegacyMCGReader.h"
#include "Callgraph.h"
#include "CgNode.h"
#include "Timing.h"
#include "Util.h"

#include <loadImbalance/LIMetaData.h>

#include <queue>

namespace metacg::pgis::io {

struct LegacyStrToNodeMapping : public StrToNodeMapping {
  LegacyStrToNodeMapping(Callgraph& cg) : cg(cg) {}

  CgNode* getNodeFromStr(const std::string& nodeStr) override { return &cg.getSingleNode(nodeStr); }

 private:
  Callgraph& cg;
};

VersionOneMetaCGReader::FuncMapT::mapped_type& VersionOneMetaCGReader::getOrInsert(const std::string& key) {
  if (functions.find(key) != functions.end()) {
    auto& fi = functions[key];
    return fi;
  } else {
    FunctionInfo fi;
    fi.functionName = key;
    functions.insert({key, fi});
    auto& rfi = functions[key];
    return rfi;
  }
}

void VersionOneMetaCGReader::buildGraph(metacg::graph::MCGManager& cgManager,
                                        VersionOneMetaCGReader::StrStrMap& potentialTargets,
                                        StrToNodeMapping& strToNode) {
  const metacg::RuntimeTimer rtt("buildGraph");
  auto console = metacg::MCGLogger::instance().getConsole();
  // Register nodes in the actual graph
  for (const auto& [k, fi] : functions) {
    console->trace("Inserting MetaCG node for function {}", k);
    auto node = &cgManager.getCallgraph()->getOrInsertNode(k);  // node pointer currently unused
    assert(node && "node is present in call graph");
    node->setVirtual(fi.isVirtual);
    node->setHasBody(fi.hasBody);
    for (const auto& c : fi.callees) {
      auto calleeNode = &cgManager.getCallgraph()->getOrInsertNode(c);
      assert(calleeNode && "calleeNode is present in call graph");
      cgManager.getCallgraph()->addEdge(*node, *calleeNode);
      auto& potTargets = potentialTargets[c];
      for (const auto& pt : potTargets) {
        auto potentialCallee = &cgManager.getCallgraph()->getOrInsertNode(pt);
        assert(potentialCallee && "potentialCallee is present in call graph");
        cgManager.getCallgraph()->addEdge(*node, *potentialCallee);
      }
    }

    std::unordered_map<std::string, std::unique_ptr<metacg::MetaData>> metadataContainer;
    for (const auto& elem : fi.namedMetadata) {
      if (auto obj = metacg::MetaData::create(elem.first, elem.second, strToNode); obj != nullptr)
        metadataContainer[elem.first] = std::move(obj);
    }
    node->setMetaDataContainer(std::move(metadataContainer));
  }
}

VersionOneMetaCGReader::StrStrMap VersionOneMetaCGReader::buildVirtualFunctionHierarchy(
    metacg::graph::MCGManager& cgManager) {
  const metacg::RuntimeTimer rtt("buildVirtualFunctionHierarchy");
  auto console = metacg::MCGLogger::instance().getConsole();
  // Now the functions map holds all the information
  std::unordered_map<std::string, std::unordered_set<std::string>> potentialTargets;
  for (const auto& [k, funcInfo] : functions) {
    if (!funcInfo.isVirtual) {
      // No virtual function, continue
      continue;
    }

    /*
     * The current function can: 1. override a function, or, 2. be overridden by a function
     *
     * (1) Add this function as potential target for any overridden function
     * (2) Add the overriding function as potential target for this function
     *
     */
    if (funcInfo.doesOverride) {
      for (const auto& overriddenFunction : funcInfo.overriddenFunctions) {
        // Adds this function as potential target to all overridden functions
        potentialTargets[overriddenFunction].insert(k);

        // In IPCG files, only the immediate overridden functions are stored currently.
        std::queue<std::string> workQ;
        std::set<std::string> visited;
        workQ.push(overriddenFunction);
        // Add this function as a potential target for all overridden functions
        while (!workQ.empty()) {
          const auto next = workQ.front();
          workQ.pop();

          const auto fi = functions[next];
          visited.insert(next);
          console->debug("In while: working on {}", next);

          potentialTargets[next].insert(k);
          for (const auto& om : fi.overriddenFunctions) {
            if (visited.find(om) == visited.end()) {
              console->debug("Adding {} to the list to process", om);
              workQ.push(om);
            }
          }
        }
      }
    }
  }

  for (const auto& [k, s] : potentialTargets) {
    std::string targets;
    for (const auto& t : s) {
      targets += t + ", ";
    }
    console->debug("Potential call targets for {}: {}", k, targets);
  }

  return potentialTargets;
}

/**
 * Version one Reader
 */
std::unique_ptr<Callgraph> VersionOneMetaCGReader::read() {
  // This version of the reader is the only one, that actually requires access to the MCGManager.
  // Manually get the cgManager via the singleton, to retain backwards compatibility
  auto& cgManager = metacg::graph::MCGManager::get();

  const metacg::RuntimeTimer rtt("Version One Reader");
  auto console = metacg::MCGLogger::instance().getConsole();

  console->trace("Reading");
  auto j = source.get();

  LegacyStrToNodeMapping strToNode(*cgManager.getCallgraph());

  for (metacg::io::json::iterator it = j.begin(); it != j.end(); ++it) {
    console->trace("Inserting node for key {}", it.key());
    auto& fi = getOrInsert(it.key());

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
  buildGraph(cgManager, potentialTargets, strToNode);
  addNumStmts(cgManager);

  // set load imbalance flags in CgNode
  for (const auto& pfi : functions) {
    std::optional<metacg::CgNode*> opt_f = &cgManager.getCallgraph()->getSingleNode(pfi.first);
    if (opt_f.has_value()) {
      metacg::CgNode* node = opt_f.value();
      node->getOrCreate<LoadImbalance::LIMetaData>();
      node->getOrCreate<LoadImbalance::LIMetaData>().setVirtual(pfi.second.isVirtual);

      if (pfi.second.visited) {
        node->getOrCreate<LoadImbalance::LIMetaData>().flag(LoadImbalance::FlagType::Visited);
      }

      if (pfi.second.irrelevant) {
        node->getOrCreate<LoadImbalance::LIMetaData>().flag(LoadImbalance::FlagType::Irrelevant);
      }
    }
  }
  return nullptr;
}

void VersionOneMetaCGReader::addNumStmts(metacg::graph::MCGManager& cgm) {
  for (const auto& [k, fi] : functions) {
    auto g = cgm.getCallgraph();
    auto node = &g->getSingleNode(fi.functionName);
    node->getOrCreate<pira::PiraOneData>();
    node->getOrCreate<pira::BaseProfileData>();
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

}  // namespace metacg::pgis::io

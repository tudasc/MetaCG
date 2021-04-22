#include "MCGReader.h"

#include "spdlog/spdlog.h"

#include <loadImbalance/LIMetaData.h>
#include <queue>

using namespace pira;

namespace MetaCG {
namespace io {

/**
 * Base class
 */

MetaCGReader::FuncMapT::mapped_type &MetaCGReader::getOrInsert(const std::string &key) {
  if (functions.find(key) != functions.end()) {
    auto &fi = functions[key];
    return fi;
  } else {
    FunctionInfo fi;
    fi.functionName = key;
    functions.insert({key, fi});
    auto &rfi = functions[key];
    return rfi;
  }
}

template <typename FieldTy, typename JsonTy>
void MetaCGReader::setIfNotNull(FieldTy &field, const JsonTy &jsonValue, const std::string key) {
  auto jsonField = jsonValue.value()[key];
  if (!jsonField.is_null()) {
    field = jsonField.template get<typename std::remove_reference<FieldTy>::type>();
  } else {
    spdlog::get("errconsole")->warn("Tried to read non-existing field {} for node.", key);
  }
}

void MetaCGReader::buildGraph(CallgraphManager &cgManager, MetaCGReader::StrStrMap &potentialTargets) {
  // Register nodes in the actual graph
  for (const auto [k, fi] : functions) {
    auto node = cgManager.findOrCreateNode(k); // node pointer currently unused
    for (const auto &c : fi.callees) {
      cgManager.putEdge(k, "", 0, c, 0, .0, 0, 0);  // regular call edges
      auto &potTargets = potentialTargets[c];
      for (const auto &pt : potTargets) {
        cgManager.putEdge(k, "", 0, pt, 0, .0, 0, 0);  // potential edges through virtual calls
      }
    }
  }
}

MetaCGReader::StrStrMap MetaCGReader::buildVirtualFunctionHierarchy(CallgraphManager &cgManager) {
  // Now the functions map holds all the information
  std::unordered_map<std::string, std::unordered_set<std::string>> potentialTargets;
  for (const auto [k, funcInfo] : functions) {
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
      for (const auto overriddenFunction : funcInfo.overriddenFunctions) {
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
          spdlog::get("console")->debug("In while: working on {}", next);

          potentialTargets[next].insert(k);
          for (const auto om : fi.overriddenFunctions) {
            if (visited.find(om) == visited.end()) {
              spdlog::get("console")->debug("Adding {} to the list to process", om);
              workQ.push(om);
            }
          }
        }
      }
    }
  }

  for (const auto [k, s] : potentialTargets) {
    std::string targets;
    for (const auto t : s) {
      targets += t + ", ";
    }
    spdlog::get("console")->debug("Potential call targets for {}: {}", k, targets);
  }

  return potentialTargets;
}

/**
 * Version one Reader
 */
void VersionOneMetaCGReader::read(CallgraphManager &cgManager) {
  spdlog::get("console")->trace("Reading");
  auto j = source.get();

  for (json::iterator it = j.begin(); it != j.end(); ++it) {
    spdlog::get("console")->trace("Inserting node for key {}", it.key());
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
  for (const auto pfi : functions) {
    std::optional<CgNodePtr> opt_f = cgManager.getCallgraph(&cgManager).findNode(pfi.first);
    if (opt_f.has_value()) {
      CgNodePtr node = opt_f.value();
      node->get<LoadImbalance::LIMetaData>()->setVirtual(pfi.second.isVirtual);

      if (pfi.second.visited) {
        node->get<LoadImbalance::LIMetaData>()->flag(LoadImbalance::FlagType::Visited);
      }

      if (pfi.second.irrelevant) {
        node->get<LoadImbalance::LIMetaData>()->flag(LoadImbalance::FlagType::Irrelevant);
      }
    }
  }
}

void VersionOneMetaCGReader::addNumStmts(CallgraphManager &cgm) {
  for (const auto [k, fi] : functions) {
    cgm.putNumberOfStatements(fi.functionName, fi.numStatements, fi.hasBody);
  }
}

/**
 * Version two Reader
 */
void VersionTwoMetaCGReader::read(CallgraphManager &cgManager) {
  auto j = source.get();

  auto mcgInfo = j["_MetaCG"];
  if (mcgInfo.is_null()) {
    spdlog::get("console")->error("Could not read version info from MetaCG file.");
    throw std::runtime_error("Could not read version info from MetaCG file");
  }
  auto mcgVersion = mcgInfo["version"].get<std::string>();
  auto generatorName = mcgInfo["generator"]["name"].get<std::string>();
  auto generatorVersion = mcgInfo["generator"]["version"].get<std::string>();
  spdlog::get("console")->info("The MetaCG (version {}) file was generated with {} (version: {})", mcgVersion,
                               generatorName, generatorVersion);
  { // raii
  std::string metaReadersStr;
  int i = 1;
  for (const auto mh : cgManager.getMetaHandlers()) {
    metaReadersStr += std::to_string(i) + ") " + mh->toolName() + "  ";
    ++i;
  }
  spdlog::get("console")->info("Executing the meta readers: {}", metaReadersStr);
  }

  auto jsonCG = j["_CG"];
  if (jsonCG.is_null()) {
    spdlog::get("console")->error("The call graph in the MetaCG file was null.");
    throw std::runtime_error("CG in MetaCG file was null.");
  }

  for (json::iterator it = jsonCG.begin(); it != jsonCG.end(); ++it) {
    auto &fi = getOrInsert(it.key());  // new entry for function it.key

    fi.functionName = it.key();

    /** Bi-directional graph information */
    std::unordered_set<std::string> callees;
    setIfNotNull(callees, it, "callees");
    fi.callees = callees;
    std::unordered_set<std::string> parents;
    setIfNotNull(parents, it, "callers");  // Different name compared to version 1.0
    fi.parents = parents;

    /** Overriding information */
    setIfNotNull(fi.isVirtual, it, "isVirtual");
    setIfNotNull(fi.doesOverride, it, "doesOverride");
    std::unordered_set<std::string> overriddenFunctions;
    setIfNotNull(overriddenFunctions, it, "overrides");
    fi.overriddenFunctions.insert(overriddenFunctions.begin(), overriddenFunctions.end());
    std::unordered_set<std::string> overriddenBy;
    setIfNotNull(overriddenBy, it, "overriddenBy");
    fi.overriddenBy.insert(overriddenBy.begin(), overriddenBy.end());

    /** Information relevant for analysis */
    setIfNotNull(fi.hasBody, it, "hasBody");

    /** 
     * Pass each attached meta reader the current json object, to see if it has meta data
     *  particular to that reader attached.
     */
    auto &jsonElem = it.value()["meta"];
    if (!jsonElem.is_null()) {
      for (const auto metaHandler : cgManager.getMetaHandlers()) {
        if (jsonElem.contains(metaHandler->toolName())) {
          metaHandler->read(jsonElem, fi.functionName);
        }
      }
    }
  }
  auto potentialTargets = buildVirtualFunctionHierarchy(cgManager);
  buildGraph(cgManager, potentialTargets);
}

void retriever::to_json(json &j, const PlacementInfo &pi) {
  spdlog::get("console")->trace("to_json from PlacementInfo called");
  j["env"]["id"] = pi.platformId;
  j["experiments"] = json::array({{"params", pi.params}, {"runtimes", pi.runtimeInSecondsPerProcess}});
  j["model"] = json{"text", pi.modelString};
}

}  // namespace io

}  // namespace MetaCG

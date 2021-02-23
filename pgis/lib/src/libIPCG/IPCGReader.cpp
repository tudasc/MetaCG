#include "IPCGReader.h"

#include "spdlog/spdlog.h"

#include <loadImbalance/LIMetaData.h>
#include <queue>

using namespace pira;

int IPCGAnal::addRuntimeDispatchCallsFromCubexProfile(CallgraphManager &ipcg, CallgraphManager &cubecg) {
  // we iterate over all nodes of profile and check whether the IPCG based graph
  // has the edge attached. If not, we insert it. This is pessimistic (safe for
  // us) in the sense that, as a worst we instrument more than needed.
  int numNewlyInsertedEdges = 0;
  for (const auto cubeNode : cubecg) {
    CgNodePtr ipcgEquivNode = ipcg.findOrCreateNode(cubeNode->getFunctionName());
    // Technically it can be, but we really want to know if so!
    assert(ipcgEquivNode != nullptr && "In the profile cannot be statically unknown nodes!");
    // now we want to compare the child nodes
    for (const auto callee : cubeNode->getChildNodes()) {
      auto res = ipcgEquivNode->getChildNodes().find(callee);  // XXX How is this comparison actually done?
      if (res == ipcgEquivNode->getChildNodes().end()) {
        const auto &[has, obj] = callee->checkAndGet<BaseProfileData>();
        auto nrOfCalls = has ? obj->getNumberOfCalls() : 0;
        // we do not have the call stored!
        ipcg.putEdge(cubeNode->getFunctionName(), "ndef", 0, callee->getFunctionName(), nrOfCalls, .0,
                     0, 0);
        numNewlyInsertedEdges++;
      }
    }
  }
  return numNewlyInsertedEdges;
}

namespace IPCGAnal {

FuncMapT::mapped_type &getOrInsert(std::string function, FuncMapT &functions) {
  if (functions.find(function) != functions.end()) {
    auto &fi = functions[function];
    return fi;
  } else {
    FunctionInfo fi;
    fi.functionName = function;
    functions.insert({function, fi});
    auto &rfi = functions[function];
    return rfi;
  }
}

void buildFromJSON(CallgraphManager &cgm, json &j, Config *c) {
  using json = nlohmann::json;

  FuncMapT functions;

  spdlog::get("console")->info("Reading IPCG file from: {}", filename);
  json j;
  {
    std::ifstream in(filename);
    if (!in.is_open()) {
      spdlog::get("errconsole")->error("Opening file failed.");
      exit(-1);
    }
    in >> j;
  }

  const auto setIfNotNull = [&](auto &field, auto jsonValue, const std::string key) {
    auto jsonField = jsonValue.value()[key];
    if (!jsonField.is_null()) {
      field = jsonField.template get<typename std::remove_reference<decltype(field)>::type>();
    } else {
      spdlog::get("errconsole")->warn("Tried to read non-existing field {} for node.", key);
    }
  };

  for (json::iterator it = j.begin(); it != j.end(); ++it) {
    auto &fi = getOrInsert(it.key(), functions);

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

  for (const auto [k, fi] : functions) {
    cgm.putNumberOfStatements(k, fi.numStatements, fi.hasBody);
    for (const auto &c : fi.callees) {
      cgm.putEdge(k, "", 0, c, 0, .0, 0, 0); // regular call edges
      cgm.putNumberOfStatements(c, functions[c].numStatements, functions[c].hasBody);
      auto &potTargets = potentialTargets[c];
      for (const auto &pt : potTargets) {
        cgm.putEdge(k, "", 0, pt, 0, .0, 0, 0); // potential edges through virtual calls
        cgm.putNumberOfStatements(pt, functions[pt].numStatements, functions[pt].hasBody);
      }
    }
  }

  // set load imbalance flags in CgNode
  for (const auto pfi : functions) {
    std::optional<CgNodePtr> opt_f = cgm.getCallgraph(&cgm).findNode(pfi.first);
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

void buildFromJSON(CallgraphManager &cgm, std::string filename, Config *c) {
  using json = nlohmann::json;

  spdlog::get("console")->info("Reading IPCG file from: {}", filename);
  json j;
  {
    std::ifstream in(filename);
    if (!in.is_open()) {
      spdlog::get("errconsole")->error("Opening file failed.");
      exit(-1);
    }
    in >> j;
  }

  buildFromJSON(cgm, j, c);
}

void IPCGAnal::retriever::to_json(json &j, const PlacementInfo &pi) {
  spdlog::get("console")->trace("to_json from PlacementInfo called");
  j["env"]["id"] = pi.platformId;
  j["experiments"] = json::array({{"params", pi.params}, {"runtimes", pi.runtimeInSecondsPerProcess}});
  j["model"] = json{"text", pi.modelString};
}

}  // namespace IPCGAnal

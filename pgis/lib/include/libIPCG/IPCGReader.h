#ifndef CUBECALLGRAPHTOOL_IPCG_READER_H
#define CUBECALLGRAPHTOOL_IPCG_READER_H

#include "CallgraphManager.h"
#include "Utility.h"

#include "nlohmann/json.hpp"

#include <loadImbalance/LIMetaData.h>
#include <string>
#include <unordered_map>
#include <unordered_set>

namespace IPCGAnal {

using json = nlohmann::json;

struct FunctionInfo {
  FunctionInfo() : functionName("_UNDEF_"), isVirtual(false), doesOverride(false), numStatements(-1), hasBody(false) {}
  std::string functionName;
  bool isVirtual;
  bool doesOverride;
  int numStatements;
  bool hasBody;
  std::unordered_set<std::string> callees;
  std::unordered_set<std::string> parents;
  std::unordered_set<std::string> overriddenFunctions;
  std::unordered_set<std::string> overriddenBy;

  // load imbalance detection data
  bool visited = false;
  bool irrelevant = false;
};

typedef std::unordered_map<std::string, FunctionInfo> FuncMapT;

int addRuntimeDispatchCallsFromCubexProfile(CallgraphManager &ipcg, CallgraphManager &cubecg);

FuncMapT::mapped_type &getOrInsert(std::string key, FuncMapT &functions);
void buildFromJSON(CallgraphManager &cgm, std::string filename, Config *c);
void buildFromJSON(CallgraphManager &cgm, json &jk, Config *c);

template <typename PropRetriever>
int doAnnotate(Callgraph &cg, PropRetriever retriever, json &j) {
  const auto functionElement = [](json &container, auto name) {
    for (json::iterator it = container.begin(); it != container.end(); ++it) {
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
void annotateJSON(Callgraph &cg, std::string filename, PropRetriever retriever) {
  json j;
  {
    std::ifstream in(filename);
    in >> j;
  }

  int annotated = IPCGAnal::doAnnotate(cg, retriever, j);
  spdlog::get("console")->trace("Annotated {} json nodes", annotated);

  {
    std::ofstream out(filename);
    out << j << std::endl;
  }
}

namespace retriever {
struct RuntimeRetriever {
  bool handles(const CgNodePtr n) {
    const auto &[hasLHS, objLHS] = n->checkAndGet<pira::BaseProfileData>();
    if (hasLHS) {
      return objLHS->getRuntimeInSeconds() > .0;
    }
    return false;
  }

  double value(const CgNodePtr n) {
    const auto &[hasLHS, objLHS] = n->checkAndGet<pira::BaseProfileData>();
    if (hasLHS) {
      return objLHS->getRuntimeInSeconds();
    }
    return .0;
  }

  std::string toolName() { return "test"; }
};

struct PlacementInfo {
  // Env section
  std::string platformId;
  // Experiments section
  std::vector<std::pair<std::string, std::vector<int>>> params;
  std::map<std::string, double> runtimeInSecondsPerProcess;
  // Model section
  std::string modelString;
};

struct ExperimentParamData {
  std::vector<std::pair<std::string, int>> params;
  std::map<std::string, double> runtimes;
};

void to_json(json &j, const PlacementInfo &pi);

struct PlacementInfoRetriever {
  bool handles(const CgNodePtr n) {
    const auto &[has, obj] = n->checkAndGet<pira::PiraTwoData>();
    if (has) {
      return obj->getExtrapModelConnector().hasModels();
    }
    return false;
  }

  PlacementInfo value(const CgNodePtr n) {
    const auto runtimePerProcess = [](CgNodePtr n) {
      auto locations = n->get<pira::BaseProfileData>()->getCgLocation();
      std::map<std::string, double> rtPerProc;
      for (auto loc : locations) {
        std::string k("P" + std::to_string(loc.getProcId()));
        auto &t = rtPerProc[k];
        t += loc.getTime();
      }
      return rtPerProc;
    };

    PlacementInfo pi;
    pi.platformId = getHostname();
    pi.params = CallgraphManager::get().getModelProvider().getConfigValues();
    pi.runtimeInSecondsPerProcess = runtimePerProcess(n);
    for (const auto &p : pi.runtimeInSecondsPerProcess) {
      std::cout << p.first << " <> " << p.second << '\n';
    }
    pi.modelString = "x^2";
    return pi;
  }

  std::string toolName() { return "plcmt_profiling"; }
};

struct PiraTwoDataRetriever {
  bool handles(const CgNodePtr n) {
    spdlog::get("console")->trace("PiraTwoRetriever:handles {}", n->getFunctionName());
    auto [has, o] = n->checkAndGet<pira::PiraTwoData>();
    if (has && o && o->hasExtrapModel()) {
      spdlog::get("console")->debug("PiraTwoDataRetriever handles node {} for model.", n->getFunctionName(),
                                    o->getRuntimeVec().size());
      return true;
    }
    if (has && o && o->getRuntimeVec().size() > 1) {
      spdlog::get("console")->debug("PiraTwoDataRetriever handles node {} for runtime.", n->getFunctionName(),
                                    o->getRuntimeVec().size());
      return true;
    }
    spdlog::get("console")->trace("Does not handle");
    return false;
  }

  pira::PiraTwoData value(const CgNodePtr n) { return *(n->get<pira::PiraTwoData>()); }

  std::string toolName() { return "PiraIIData"; }
};

}  // namespace retriever

}  // namespace IPCGAnal

#endif

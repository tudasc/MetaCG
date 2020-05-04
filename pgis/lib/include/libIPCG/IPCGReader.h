#ifndef CUBECALLGRAPHTOOL_IPCG_READER_H
#define CUBECALLGRAPHTOOL_IPCG_READER_H

#include "CallgraphManager.h"

#include "nlohmann/json.hpp"

#include <string>
#include <unordered_map>
#include <unordered_set>

namespace IPCGAnal {

using json = nlohmann::json;

struct FunctionInfo {
  FunctionInfo() : functionName("_UNDEF_"), isVirtual(false), doesOverride(false), numStatements(-1) {}
  std::string functionName;
  bool isVirtual;
  bool doesOverride;
  int numStatements;
  std::unordered_set<std::string> callees;
  std::unordered_set<std::string> parents;
  std::unordered_set<std::string> overriddenFunctions;
};

typedef std::unordered_map<std::string, FunctionInfo> FuncMapT;

void build(CallgraphManager &cg, std::string filepath, Config *c);

int addRuntimeDispatchCallsFromCubexProfile(CallgraphManager &ipcg, CallgraphManager &cubecg);

FuncMapT::mapped_type &getOrInsert(std::string key, FuncMapT &functions);
void buildFromJSON(CallgraphManager &cgm, std::string filename, Config *c);

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

  int annotated = doAnnotate(cg, retriever, j);
  std::cout << "Annotated " << annotated << " nodes / json elements" << std::endl;

  {
    std::ofstream out(filename);
    out << j << std::endl;
  }
}

namespace retriever {
struct RuntimeRetriever {
  bool handles(const CgNodePtr n) { return n->getRuntimeInSeconds() > .0; }

  double value(const CgNodePtr n) { return n->getRuntimeInSeconds(); }

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

void to_json(json &j, const PlacementInfo &pi) {
  std::cout << "to_json from PlacementInfo called" << std::endl;
  j["env"] = json{"id", pi.platformId};
  j["experiments"] = json{{"params", pi.params}, {"runtimes", pi.runtimeInSecondsPerProcess}};
  j["model"] = json{"text", pi.modelString};
  std::cout << j << std::endl;
}

struct PlacementInfoRetriever {
  bool handles(const CgNodePtr n) {
    return n->getExtrapModelConnector().hasModels();
  }

  PlacementInfo value(const CgNodePtr n) {
    const auto runtimePerProcess = [](CgNodePtr n) {
      auto locations = n->getCgLocation();
      std::map<std::string, double> rtPerProc;
      for (auto loc : locations) {
        std::string k("P" + std::to_string(loc.get_procId()));
        auto &t = rtPerProc[k];
        t += loc.get_time();
      }
      return rtPerProc;
    };

    PlacementInfo pi;
    pi.platformId = "Lichtenberg I Phase II";
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

}  // namespace retriever

}  // namespace IPCGAnal

#endif

/**
 * File: MCGReader.h
 * License: Part of the MetaCG project. Licensed under BSD 3 clause license. See LICENSE.txt file at https://github.com/tudasc/metacg/LICENSE.txt
 */

#ifndef CUBECALLGRAPHTOOL_IPCG_READER_H
#define CUBECALLGRAPHTOOL_IPCG_READER_H

#include "CallgraphManager.h"
#include "Utility.h"

#include "MetaDataHandler.h"

#include "nlohmann/json.hpp"

#include <loadImbalance/LIMetaData.h>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>

namespace MetaCG {

namespace io {

using json = nlohmann::json;

/**
 * Abstraction for the source of the JSON to read-in
 * Meant to be subclassed.
 */
struct ReaderSource {
  /**
   * Returns a json object of the call graph.
   */
  virtual nlohmann::json get() const = 0;
};

/**
 * Wraps a file as the source of the JSON.
 * If the file does not exists, prints error and exits the program.
 */
struct FileSource : ReaderSource {
  FileSource(const std::string filename) : filename(filename) {}
  /**
   * Reads the json file with filename (provided at object construction)
   * and returns the json object.
   */
  virtual nlohmann::json get() const override {
    spdlog::get("console")->info("Reading MetaCG file from: {}", filename);
    nlohmann::json j;
    {
      std::ifstream in(filename);
      if (!in.is_open()) {
        spdlog::get("errconsole")->error("Opening file {} failed.", filename);
        throw std::runtime_error("Opening file failed");
      }
      in >> j;
    }

    return j;
  };
  const std::string filename;
};

/**
 * Wraps existing JSON object as source.
 * Currently only used in unit tests.
 */
struct JsonSource : ReaderSource {
  JsonSource(nlohmann::json j) : json(j) {}
  virtual nlohmann::json get() const override { return json; }
  nlohmann::json json;
};

/**
 * Base class to read MetaCG files.
 *
 * Previously known as IPCG files, the MetaCG files are the serialized versions of the call graph.
 * This class implements basic functionality and is meant to be subclassed for different file versions.
 */
class MetaCGReader {
  /**
   * Internal data structure to create the MetaCG structure
   */
  struct FunctionInfo {
    FunctionInfo()
        : functionName("_UNDEF_"), isVirtual(false), doesOverride(false), numStatements(-1), hasBody(false) {}
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

 public:
  typedef std::unordered_map<std::string, FunctionInfo> FuncMapT;
  typedef std::unordered_map<std::string, std::unordered_set<std::string>> StrStrMap;
  // using json = nlohmann::json;
  /**
   * filename path to file
   */
  MetaCGReader(ReaderSource &src) : source(src) {}
  /**
   * CallgraphManager object to be filled with the CG
   */
  virtual void read(CallgraphManager &cgManager) = 0;

 protected:
  /**
   * Gets or inserts new node into the functions LUT
   */
  FuncMapT::mapped_type &getOrInsert(const std::string &key);

  /**
   * Checks if the jsonValue contains a value for key and sets field accordingly
   */
  template <typename FieldTy, typename JsonTy>
  void setIfNotNull(FieldTy &field, const JsonTy &jsonValue, const std::string key);

  /**
   * Build the virtual function hierarchy in PGIS using the functions map
   */
  StrStrMap buildVirtualFunctionHierarchy(CallgraphManager &cgManager);

  /**
   * Inserts all nodes in the function map into the final call-graph.
   * Uses potentialTargets to include all potential virtual call targets for virtual functions.
   */
  void buildGraph(CallgraphManager &cgManager, StrStrMap &potentialTargets);

  /* Functions LUT: needed for construction of MetaCG */
  FuncMapT functions;

  /**
   * Abstraction from where to read-in the JSON.
   */
  const ReaderSource &source;

 private:
  // filename of the MetaCG this instance parses
  const std::string filename;
};

/**
 * Class to read MetaCG files in file format v1.0.
 * The file format is also typically referred to as IPCG files.
 */
class VersionOneMetaCGReader : public MetaCGReader {
 public:
  VersionOneMetaCGReader(ReaderSource &source) : MetaCGReader(source) {}
  void read(CallgraphManager &cgManager) override;

 private:
  void addNumStmts(CallgraphManager &cgm);
};

/**
 * Class to read MetaCG files in file format v2.0.
 * The format contains the 'meta' field for tools to export information.
 */
class VersionTwoMetaCGReader : public MetaCGReader {
 public:
  VersionTwoMetaCGReader(ReaderSource &source) : MetaCGReader(source) {}
  void read(CallgraphManager &cgManager) override;
};


/*
 *
 * (Old?) Annotation mechanism
 *
 */

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

  int annotated = MetaCG::io::doAnnotate(cg, retriever, j);
  spdlog::get("console")->trace("Annotated {} json nodes", annotated);

  {
    std::ofstream out(filename);
    out << j << std::endl;
  }
}

namespace retriever {

/**
 * FIXME: Not sure if these are useful (or even used) to any extent!
 */
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
/* XXX End of FIXME */

}  // namespace retriever

}  // namespace io

}  // namespace MetaCG

#endif

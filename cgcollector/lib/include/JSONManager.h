#ifndef CGCOLLECTOR_JSONMANAGER_H
#define CGCOLLECTOR_JSONMANAGER_H

#include "MetaInformation.h"
#include "config.h"

#include "AliasAnalysis.h"
#include "nlohmann/json.hpp"

#include <fstream>
#include <iostream>
#include <set>
#include <unordered_set>

using FunctionNames = std::set<std::string>;

/** Parses the file 'filename' into the json object 'callgraph'
 *
 * @param filename
 * @param callgraph
 */
inline void readIPCG(const std::string& filename, nlohmann::json& callgraph) {
  std::ifstream file(filename);
  if (!file.is_open()) {
    std::cerr << "[Error] Unable to open inputfile " << filename << std::endl;
    exit(-1);
  }
  file >> callgraph;
}

inline void writeIPCG(const std::string& filename, const nlohmann::json& callgraph) {
  std::ofstream file(filename);
  if (!file.is_open()) {
    std::cerr << "[Error] Unable to open outputfile " << filename << std::endl;
    exit(-1);
  }
  file << callgraph;
}

inline void attachFormatTwoHeader(nlohmann::json& j) {
  std::string cgcMajorVersion = std::to_string(CGCollector_VERSION_MAJOR);
  std::string cgcMinorVersion = std::to_string(CGCollector_VERSION_MINOR);
  std::string cgcVersion{cgcMajorVersion + '.' + cgcMinorVersion};
  j = {{"_MetaCG", {}}, {"_CG", {}}};
  j["_MetaCG"] = {{"version", "2.0"},
                  {"generator", {{"name", "CGCollector"}, {"version", cgcVersion}, {"sha", MetaCG_GIT_SHA}}}};
}

/**
 *
 * @param callgraph
 * @param nodeName
 * @param callees
 * @param callers
 * @param overriddenBy
 * @param overriddenFunctions
 * @param isVirtual
 * @param doesOverride
 * @param hasBody
 * @param version
 * @param directInsertWorkaround Workaround to enable CGValidate to work with the metacg version 2 file format
 */
inline void insertNode(nlohmann::json& callgraph, const std::string& nodeName, const FunctionNames& callees,
                       const FunctionNames& callers, const FunctionNames& overriddenBy,
                       const FunctionNames& overriddenFunctions, const bool isVirtual, const bool doesOverride,
                       const bool hasBody, int version, const bool directInsertWorkaround = false) {
  if (version == 1) {
    callgraph[nodeName] = {{"callees", callees},
                           {"isVirtual", isVirtual},
                           {"doesOverride", doesOverride},
                           {"overriddenFunctions", overriddenFunctions},
                           {"overriddenBy", overriddenBy},
                           {"parents", callers},
                           {"hasBody", hasBody}};
  } else if (version == 2) {
    if (!directInsertWorkaround) {
      callgraph["_CG"][nodeName] = {{"callees", callees},
                                    {"isVirtual", isVirtual},
                                    {"doesOverride", doesOverride},
                                    {"overrides", overriddenFunctions},
                                    {"overriddenBy", overriddenBy},
                                    {"callers", callers},
                                    {"hasBody", hasBody}};
    } else {
      callgraph[nodeName] = {{"callees", callees},
                             {"isVirtual", isVirtual},
                             {"doesOverride", doesOverride},
                             {"overrides", overriddenFunctions},
                             {"overriddenBy", overriddenBy},
                             {"callers", callers},
                             {"hasBody", hasBody}};
    }
  }
}

/**
 *
 * @param callgraph
 * @param nodeName
 * @param version
 * @param directInsertWorkaround Workaround to enable CGValidate to work with the metacg version 2 file format
 */
inline void insertDefaultNode(nlohmann::json& callgraph, const std::string& nodeName, const int version = 1,
                              const bool directInsertWorkaround = false) {
  const FunctionNames empty;
  insertNode(callgraph, nodeName, empty, empty, empty, empty, false, false, false, version, directInsertWorkaround);
}

struct FunctionInfo {
  FunctionInfo() : functionName("_UNDEF_"), isVirtual(false), doesOverride(false), numStatements(-1), hasBody(false) {}
  ~FunctionInfo() = default;

  bool compareStructure(const FunctionInfo& other) const {
    bool isEqual = true;
    isEqual &= (isVirtual == other.isVirtual);
    isEqual &= (doesOverride == other.doesOverride);
    isEqual &= (hasBody == other.hasBody);

    isEqual &= (functionName == other.functionName);
    isEqual &= (callees == other.callees);
    isEqual &= (parents == other.parents);
    isEqual &= (overriddenFunctions == other.overriddenFunctions);
    isEqual &= (overriddenBy == other.overriddenBy);

    return isEqual;
  }

  /**
   * Checks if a function contains the callees that are expected
   * @param required Functions that need to be called by the function
   * @param blacklist Functions that are not allowed to be called by the function
   * @param strict If strict is set to true, only calls to functions in required are allowed
   * @return
   */
  bool check_callees(const std::set<std::string>& required, const std::set<std::string>& blacklist, bool strict) const {
    if (strict) {
      return callees == required;
    } else {
      bool containsRequired = std::includes(callees.begin(), callees.end(), required.begin(), required.end());
      if (containsRequired) {
        return std::none_of(blacklist.begin(), blacklist.end(),
                            [this](const auto& be) { return callees.find(be) != callees.end(); });
      }
      return false;
    }
  }

  bool compareMeta(const FunctionInfo& other) const {
    bool isEqual = true;
    for (const auto& [k, mi] : metaInfo) {
      const auto oIt = other.metaInfo.find(k);
      if (oIt != other.metaInfo.end()) {
        auto otherMI = *oIt;
        // equals does hard casts -> if different meta information for equal key => bad anyway
        isEqual &= mi->equals(otherMI.second);
      } else {
        isEqual &= false;
      }
    }
    return isEqual;
  }

  std::string functionName;
  bool isVirtual;
  bool doesOverride;
  int numStatements;
  bool hasBody;
  FunctionNames callees;
  FunctionNames parents;
  FunctionNames overriddenFunctions;
  FunctionNames overriddenBy;
  std::unordered_map<std::string, MetaInformation*> metaInfo;
};

typedef std::map<std::string, FunctionInfo> FuncMapT;

nlohmann::json buildFromJSON(FuncMapT& functionMap, const std::string& filename);

nlohmann::json buildFromJSONv2(FuncMapT& functionMap, const std::string& filename, nlohmann::json* CompleteJsonOut);

#endif /* ifndef CGCOLLECTOR_JSONMANAGER_H */

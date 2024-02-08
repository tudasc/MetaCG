/**
 * File: CgNodeMetaData.h
 * License: Part of the MetaCG project. Licensed under BSD 3 clause license. See LICENSE.txt file at
 * https://github.com/tudasc/metacg/LICENSE.txt
 */

#ifndef PIRA_CGNODE_METADATA_H
#define PIRA_CGNODE_METADATA_H

// clang-format off
// Graph library
#include "CgNodePtr.h"
#include "CgNode.h"
#include "Utility.h"
#include "MetaData.h"
#include "LoggerUtil.h"

// PGIS library
#include "ExtrapConnection.h"
#include "config/GlobalConfig.h"

#include "CgLocation.h"

#include <string>
#include <map>

#include "nlohmann/json.hpp"

// clang-format on

namespace pira {

inline void assert_pira_one_data() { assert(false && "PIRA I data should be available in node"); }

typedef unsigned long long Statements;

/**
 * This class holds basic profile information, e.g., from reading a CUBE
 */
class BaseProfileData : public metacg::MetaData::Registrar<BaseProfileData> {
 public:
  static constexpr const char *key = "BaseProfileData";
  BaseProfileData() = default;
  BaseProfileData(const nlohmann::json &j) {
    metacg::MCGLogger::instance().getConsole()->trace("Running BaseProfileDataHandler::read");
    if (j.is_null()) {
      metacg::MCGLogger::instance().getConsole()->error("Could not retrieve metadata for {}", key);
      return;
    }
    auto jsonNumCalls = j["numCalls"].get<unsigned long long int>();
    auto rtInSeconds = j["timeInSeconds"].get<double>();
    auto inclRtInSeconds = j["inclusiveRtInSeconds"].get<double>();
    setNumberOfCalls(jsonNumCalls);
    setRuntimeInSeconds(rtInSeconds);
    setInclusiveRuntimeInSeconds(inclRtInSeconds);
  }
  nlohmann::json to_json() const final {
    return nlohmann::json{{"numCalls", getNumberOfCalls()},
                          {"timeInSeconds", getRuntimeInSeconds()},
                          {"inclusiveRtInSeconds", getInclusiveRuntimeInSeconds()}};
  };

  virtual const char *getKey() const final { return key; }

  // Regular profile data
  // Warning: This function is *not* used by the Cube reader
  void setCallData(metacg::CgNode *parentNode, unsigned long long calls, double timeInSeconds,
                   double inclusiveTimeInSeconds, int threadId, int procId) {
    callFrom[parentNode] += calls;
    this->timeInSeconds += timeInSeconds;
    this->inclTimeInSeconds += inclusiveTimeInSeconds;
    this->threadId = threadId;
    this->processId = procId;
    this->cgLoc.emplace_back(CgLocation(timeInSeconds, inclusiveTimeInSeconds, threadId, procId, calls));
  }
  unsigned long long getNumberOfCalls() const { return this->numCalls; }
  void setNumberOfCalls(unsigned long long nrCall) { this->numCalls = nrCall; }

  void addCalls(unsigned long long nrCall) { this->numCalls += nrCall; }

  double getRuntimeInSeconds() const { return this->timeInSeconds; }
  void setRuntimeInSeconds(double newRuntimeInSeconds) { this->timeInSeconds = newRuntimeInSeconds; }

  void addRuntime(double runtime) {
    assert(runtime >= 0);
    this->timeInSeconds += runtime;
  }

  void setInclusiveRuntimeInSeconds(double newInclusiveTimeInSeconds) {
    assert(inclTimeInSeconds == 0 && "You probably don't want to overwrite the incl runtime");
    inclTimeInSeconds = newInclusiveTimeInSeconds;
  }

  void addInclusiveRuntimeInSeconds(double newInclusiveTimeInSeconds) {
    assert(newInclusiveTimeInSeconds >= 0);
    inclTimeInSeconds += newInclusiveTimeInSeconds;
  }

  double getInclusiveRuntimeInSeconds() const { return this->inclTimeInSeconds; }
  unsigned long long getNumberOfCallsWithCurrentEdges() const {
    auto v = 0ull;
    for (const auto &p : callFrom) {
      v += p.second;
    }
    return v;
  }

  std::map<std::string, unsigned long long> getCallsFromParents() const {
    std::map<std::string, unsigned long long> result;
    for (const auto &p : callFrom) {
      if (p.first) {
        result[p.first->getFunctionName()] = p.second;
      }
    }
    return result;
  }

  void addNumberOfCallsFrom(metacg::CgNode *parentNode, unsigned long long calls) { callFrom[parentNode] += calls; }

  const std::vector<CgLocation> &getCgLocation() const { return cgLoc; }

  void pushCgLocation(CgLocation toPush) { this->cgLoc.push_back(toPush); }

 private:
  unsigned long long numCalls{0};
  double timeInSeconds{.0};
  double inclTimeInSeconds{.0};
  int threadId{0};
  int processId{0};
  std::unordered_map<metacg::CgNode *, unsigned long long> callFrom;
  std::vector<CgLocation> cgLoc;
};

/**
 * This class holds data relevant to the PIRA I analyses.
 * Most notably, it offers the number of statements and the principal (dominant) runtime node
 */
class PiraOneData : public metacg::MetaData::Registrar<PiraOneData> {
 public:
  static constexpr const char *key = "numStatements";
  PiraOneData() = default;
  explicit PiraOneData(const nlohmann::json &j) {
    metacg::MCGLogger::instance().getConsole()->trace("Running PiraOneMetaDataRetriever::read from json");
    if (j.is_null()) {
      metacg::MCGLogger::instance().getConsole()->error("Could not retrieve meta data for {}", key);
      return;
    }
    auto jsonNumStmts = j.get<long long int>();
    metacg::MCGLogger::instance().getConsole()->debug("Read {} stmts from file", jsonNumStmts);
    setNumberOfStatements(jsonNumStmts);
  }

  nlohmann::json to_json() const final {
    nlohmann::json j;
    j["numStatements"] = getNumberOfStatements();
    return j;
  };

  virtual const char *getKey() const final { return key; }

  void setNumberOfStatements(int numStmts) { this->numStmts = numStmts; }
  int getNumberOfStatements() const { return this->numStmts; }
  void setHasBody(bool hasBody = true) { this->hasBody = hasBody; }
  bool getHasBody() const { return this->hasBody; }
  void setDominantRuntime(bool dominantRuntime = true) { this->dominantRuntime = dominantRuntime; }
  bool isDominantRuntime() const { return this->dominantRuntime; }
  void setComesFromCube(bool fromCube = true) { this->wasInPreviousProfile = fromCube; }
  bool comesFromCube() const { return this->wasInPreviousProfile; }
  bool inPreviousProfile() const { return wasInPreviousProfile; }

 private:
  bool wasInPreviousProfile{false};
  bool dominantRuntime{false};
  bool hasBody{false};
  int numStmts{0};
};

template <typename T>
inline void setPiraOneData(T node, int numStmts = 0, bool hasBody = false, bool dominantRuntime = false,
                           bool inPrevProfile = false) {
  const auto &[has, data] = node->template checkAndGet<PiraOneData>();
  if (has) {
    data->setNumberOfStatements(numStmts);
    data->setHasBody(hasBody);
    data->setDominantRuntime(dominantRuntime);
    data->setComesFromCube(inPrevProfile);
  } else {
    assert_pira_one_data();
  }
}

inline void to_json(nlohmann::json &j, const PiraOneData &data) {
  j = nlohmann::json{{"numStatements", data.getNumberOfStatements()}};
}

/**
 * This class holds data relevant to the PIRA II anslyses.
 * Most notably it encapsulates the Extra-P peformance models
 */
class PiraTwoData : public metacg::MetaData::Registrar<PiraTwoData> {
 public:
  static constexpr const char *key = "PiraTwoData";

  explicit PiraTwoData() : epCon({}, {}), params(), rtVec(), numReps(0) {}
  explicit PiraTwoData(const nlohmann::json &j) : epCon({}, {}), params(), rtVec(), numReps(0) {
    metacg::MCGLogger::instance().getConsole()->warn(
        "Read PiraTwoData from json currently not implemented / supported");
  };
  explicit PiraTwoData(const extrapconnection::ExtrapConnector &ec) : epCon(ec), params(), rtVec(), numReps(0) {}
  PiraTwoData(const PiraTwoData &other)
      : epCon(other.epCon), params(other.params), rtVec(other.rtVec), numReps(other.numReps) {
    metacg::MCGLogger::instance().getConsole()->trace("PiraTwo Copy CTor\n\tother: {}\n\tThis: {}", other.rtVec.size(),
                                                      rtVec.size());
  }

  nlohmann::json to_json() const final {
    nlohmann::json j;
    if (getExtrapModel() == nullptr) {
      metacg::MCGLogger::instance().getConsole()->error(
          "PiraTwoData can not be exported, no connected ExtraP model exists");
      return j;
    }
    auto &gOpts = ::pgis::config::GlobalConfig::get();
    auto rtOnly = gOpts.getAs<bool>("runtime-only");

    auto rtAndParams = utils::valTup(getRuntimeVec(), getExtrapParameters(), getNumReps());
    nlohmann::json experiments;
    for (auto elem : rtAndParams) {
      nlohmann::json exp{};
      exp["runtime"] = elem.first;
      exp[elem.second.first] = elem.second.second;
      experiments += exp;
    }
    if (!rtOnly) {
      j = nlohmann::json{{"experiments", experiments},
                         {"model", getExtrapModel()->getAsString(getExtrapModelConnector().getParamList())}};
    } else {
      j = nlohmann::json{{"experiments", experiments}};
    }
    metacg::MCGLogger::instance().getConsole()->debug("PiraTwoData to_json:\n{}", j.dump());
    return j;
  };

  virtual const char *getKey() const final { return key; }

  void setExtrapModelConnector(extrapconnection::ExtrapConnector epCon) { this->epCon = epCon; }
  extrapconnection::ExtrapConnector &getExtrapModelConnector() { return epCon; }
  const extrapconnection::ExtrapConnector &getExtrapModelConnector() const { return epCon; }

  void setExtrapParameters(std::vector<std::pair<std::string, std::vector<int>>> params) { this->params = params; }
  const std::vector<std::pair<std::string, std::vector<int>>> &getExtrapParameters() const { return this->params; }

  void addToRuntimeVec(double runtime) { this->rtVec.push_back(runtime); }
  const std::vector<double> &getRuntimeVec() const { return this->rtVec; }

  const std::unique_ptr<EXTRAP::Function> &getExtrapModel() const { return epCon.getEPModelFunction(); }

  bool hasExtrapModel() const { return epCon.hasModels(); }

  int getNumReps() const { return numReps; }

 private:
  extrapconnection::ExtrapConnector epCon;
  std::vector<std::pair<std::string, std::vector<int>>> params;
  std::vector<double> rtVec;
  int numReps;
};

class FilePropertiesMetaData : public metacg::MetaData::Registrar<FilePropertiesMetaData> {
 public:
  static constexpr const char *key = "fileProperties";
  FilePropertiesMetaData() : origin("INVALID"), fromSystemInclude(false), lineNumber(0) {}
  explicit FilePropertiesMetaData(const nlohmann::json &j) {
    if (j.is_null()) {
      metacg::MCGLogger::instance().getConsole()->error("Could not retrieve meta data for {}", key);
      return;
    }

    std::string fileOrigin = j["origin"].get<std::string>();
    bool isFromSystemInclude = j["systemInclude"].get<bool>();
    origin = fileOrigin;
    fromSystemInclude = isFromSystemInclude;
  }

  nlohmann::json to_json() const final {
    nlohmann::json j;
    j["origin"] = origin;
    j["systemInclude"] = fromSystemInclude;
    return j;
  };

  virtual const char *getKey() const final { return key; }

  std::string origin;
  bool fromSystemInclude;
  int lineNumber;
};

class CodeStatisticsMetaData : public metacg::MetaData::Registrar<CodeStatisticsMetaData> {
 public:
  static constexpr const char *key = "codeStatistics";
  CodeStatisticsMetaData() = default;
  explicit CodeStatisticsMetaData(const nlohmann::json &j) {
    if (j.is_null()) {
      metacg::MCGLogger::instance().getConsole()->error("Could not retrieve meta data for {}", key);
      return;
    }
    int jNumVars = j["numVars"].get<int>();
    numVars = jNumVars;
  }

  nlohmann::json to_json() const final {
    nlohmann::json j;
    j["numVars"] = numVars;
    return j;
  };

  virtual const char *getKey() const final { return key; }

  int numVars{0};
};

class NumConditionalBranchMetaData : public metacg::MetaData::Registrar<NumConditionalBranchMetaData> {
 public:
  static constexpr const char *key = "numConditionalBranches";
  NumConditionalBranchMetaData() = default;
  explicit NumConditionalBranchMetaData(const nlohmann::json &j) {
    if (j.is_null()) {
      metacg::MCGLogger::instance().getConsole()->error("Could not retrieve meta data for {}", key);
      return;
    }
    int numberOfConditionalBranches = j.get<int>();
    numConditionalBranches = numberOfConditionalBranches;
  }

  nlohmann::json to_json() const final { return numConditionalBranches; };

  virtual const char *getKey() const final { return key; }

  int numConditionalBranches{0};
};

class NumOperationsMetaData : public metacg::MetaData::Registrar<NumOperationsMetaData> {
 public:
  NumOperationsMetaData() = default;
  static constexpr const char *key = "numOperations";

  explicit NumOperationsMetaData(const nlohmann::json &j) {
    if (j.is_null()) {
      metacg::MCGLogger::instance().getConsole()->error("Could not retrieve meta data for {}", key);
      return;
    }
    int jNumberOfIntOps = j["numberOfIntOps"].get<int>();
    int jNumberOfFloatOps = j["numberOfFloatOps"].get<int>();
    int jNumberOfControlFlowOps = j["numberOfControlFlowOps"].get<int>();
    int jNumberOfMemoryAccesses = j["numberOfMemoryAccesses"].get<int>();
    numberOfIntOps = jNumberOfIntOps;
    numberOfFloatOps = jNumberOfFloatOps;
    numberOfControlFlowOps = jNumberOfControlFlowOps;
    numberOfMemoryAccesses = jNumberOfMemoryAccesses;
  }
  nlohmann::json to_json() const final {
    nlohmann::json j;
    j["numberOfIntOps"] = numberOfIntOps;
    j["numberOfFloatOps"] = numberOfFloatOps;
    j["numberOfControlFlowOps"] = numberOfControlFlowOps;
    j["numberOfMemoryAccesses"] = numberOfMemoryAccesses;
    return j;
  };

  virtual const char *getKey() const final { return key; }

  int numberOfIntOps{0};
  int numberOfFloatOps{0};
  int numberOfControlFlowOps{0};
  int numberOfMemoryAccesses{0};
};

class LoopDepthMetaData : public metacg::MetaData::Registrar<LoopDepthMetaData> {
 public:
  static constexpr const char *key = "loopDepth";
  LoopDepthMetaData() = default;
  explicit LoopDepthMetaData(const nlohmann::json &j) {
    if (j.is_null()) {
      metacg::MCGLogger::instance().getConsole()->error("Could not retrieve meta data for {}", key);
      return;
    }
    loopDepth = j.get<int>();
  }

  nlohmann::json to_json() const final { return loopDepth; };

  virtual const char *getKey() const final { return key; }

  int loopDepth{0};
};

class GlobalLoopDepthMetaData : public metacg::MetaData::Registrar<GlobalLoopDepthMetaData> {
 public:
  GlobalLoopDepthMetaData() = default;
  static constexpr const char *key = "globalLoopDepth";

  explicit GlobalLoopDepthMetaData(const nlohmann::json &j) {
    if (j.is_null()) {
      metacg::MCGLogger::instance().getConsole()->error("Could not retrieve meta data for {}", key);
      return;
    }
    globalLoopDepth = j.get<int>();
  }
  nlohmann::json to_json() const final { return globalLoopDepth; };

  virtual const char *getKey() const final { return key; }

  int globalLoopDepth{0};
};

class InlineMetaData : public metacg::MetaData::Registrar<InlineMetaData> {
 public:
  static constexpr const char *key = "inlineInfo";
  virtual const char *getKey() const final { return key; }
  InlineMetaData() = default;
  explicit InlineMetaData(const nlohmann::json &j) {
    if (j.is_null()) {
      metacg::MCGLogger::instance().getConsole()->error("Could not retrieve meta data for {}", key);
      return;
    }
    markedInline = j["markedInline"].get<bool>();
    likelyInline = j["likelyInline"].get<bool>();
    isTemplate = j["isTemplate"].get<bool>();
    markedAlwaysInline = j["markedAlwaysInline"].get<bool>();
  }

  nlohmann::json to_json() const final {
    nlohmann::json j;
    j["markedInline"] = markedInline;
    j["likelyInline"] = likelyInline;
    j["isTemplate"] = isTemplate;
    j["markedAlwaysInline"] = markedAlwaysInline;
    return j;
  }

  bool markedInline{false};
  bool likelyInline{false};
  bool markedAlwaysInline{false};
  bool isTemplate{false};
};

// COPIED FROM cgcollector/lib/include/MetaInformation.h  Needs to be kept in sync
struct CodeRegion {
  std::string parent;
  std::set<std::string> functions;
  double parentCalls;
  NLOHMANN_DEFINE_TYPE_INTRUSIVE(CodeRegion, parent, functions, parentCalls)
};

using CalledFunctionType = std::map<std::string, std::set<std::pair<double, std::string>>>;
using CodeRegionsType = std::map<std::string, CodeRegion>;

class CallCountEstimationMetaData : public metacg::MetaData::Registrar<CallCountEstimationMetaData> {
 public:
  static constexpr const char *key = "estimateCallCount";
  virtual const char *getKey() const final { return key; }
  CallCountEstimationMetaData() = default;
  explicit CallCountEstimationMetaData(const nlohmann::json &j) {
    if (j.is_null()) {
      metacg::MCGLogger::instance().getConsole()->error("Could not retrieve meta data for {}", key);
      return;
    }
    calledFunctions = j["calls"].get<pira::CalledFunctionType>();
    codeRegions = j["codeRegions"].get<pira::CodeRegionsType>();
  }

  nlohmann::json to_json() const final {
    nlohmann::json j;
    j["calls"] = calledFunctions;
    j["codeRegions"] = codeRegions;
    return j;
  }

  CalledFunctionType
      calledFunctions;  // Maps the name of called function to the estimate of local calls to it and the regions in
                        // which they occur. the region name is empty if its directly in the function
  CodeRegionsType codeRegions;
};

struct InstumentationInfo {
  inline bool operator<(const InstumentationInfo &rhs) const {
    return std::tie(infoPerCall, inclusiveStmtCount, exclusiveStmtCount, callsFromParents) <
           std::tie(rhs.infoPerCall, rhs.inclusiveStmtCount, rhs.exclusiveStmtCount, rhs.callsFromParents);
  };
  double callsFromParents;
  unsigned long inclusiveStmtCount;
  unsigned long exclusiveStmtCount;
  double infoPerCall;
  bool Init = false;  // Ignored for comparisons
  InstumentationInfo(double callsFromParents, unsigned long inclusiveStmtCount, unsigned long exclusiveStmtCount) {
    this->callsFromParents = callsFromParents;
    this->exclusiveStmtCount = exclusiveStmtCount;
    this->inclusiveStmtCount = inclusiveStmtCount;
    this->infoPerCall = inclusiveStmtCount / callsFromParents;
    this->Init = true;
  };
  InstumentationInfo() = default;
  bool operator==(const InstumentationInfo &rhs) const {
    return std::tie(callsFromParents, inclusiveStmtCount, exclusiveStmtCount, infoPerCall) ==
           std::tie(rhs.callsFromParents, rhs.inclusiveStmtCount, rhs.exclusiveStmtCount, rhs.infoPerCall);
  }
};

/**
 * This is just for temporay data and does not get serialized
 */
class TemporaryInstrumentationDecisionMetadata
    : public metacg::MetaData::Registrar<TemporaryInstrumentationDecisionMetadata> {
 public:
  static constexpr const char *key = "TemporaryInstrumentationDecisionMetadata";
  virtual const char *getKey() const final { return key; }
  TemporaryInstrumentationDecisionMetadata() = default;
  explicit TemporaryInstrumentationDecisionMetadata(const nlohmann::json &j) {
    (void)j;
    // This is just temporary data
  }
  nlohmann::json to_json() const final { return {}; }
  InstumentationInfo info;
  bool isKicked = false;
  bool parentHasHighExclusiveRuntime = false;
};

/**
 * Class holding instrumentation results, i.e. cube information after running the instrumented binary
 */
class InstrumentationResultMetaData : public metacg::MetaData::Registrar<InstrumentationResultMetaData> {
 public:
  static constexpr const char *key = "instrumentationResult";
  virtual const char *getKey() const final { return key; }

  InstrumentationResultMetaData() = default;
  explicit InstrumentationResultMetaData(const nlohmann::json &j) {
    if (j.is_null()) {
      metacg::MCGLogger::instance().getConsole()->error("Could not retrieve meta data for {}", key);
      return;
    }
    callCount = j["calls"];
    runtime = j["runtime"];
    timePerCall = j["timePerCall"];
    isExclusiveRuntime = j["exclusive"];
    inclusiveRunTimeCube = j["inclusiveRuntimeCube"];
    inclusiveTimePerCallCube = j["inclusiveTimePerCallCube"];
    inclusiveRunTimeSum = j["inclusiveRuntimeSum"];
    inclusiveTimePerCallSum = j["inclusiveTimePerCallSum"];
    shouldBeInstrumented = j["shouldBeInstrumented"];
    callsFromParents = j["callsFromParents"];
  }

  nlohmann::json to_json() const final {
    nlohmann::json j;
    j["calls"] = callCount;
    j["runtime"] = runtime;
    j["timePerCall"] = timePerCall;
    j["exclusive"] = isExclusiveRuntime;
    j["inclusiveRuntimeCube"] = inclusiveRunTimeCube;
    j["inclusiveTimePerCallCube"] = inclusiveTimePerCallCube;
    j["inclusiveRuntimeSum"] = inclusiveRunTimeSum;
    j["inclusiveTimePerCallSum"] = inclusiveTimePerCallSum;
    j["shouldBeInstrumented"] = shouldBeInstrumented;
    j["callsFromParents"] = callsFromParents;
    return j;
  }

  unsigned long long callCount{0};
  std::map<std::string, unsigned long long> callsFromParents;
  /**
   * The runtime of just the node. May be inclusive of other nodes depending on if they are instrumented or not
   */
  double runtime{0};
  /**
   * based on runtime
   */
  double timePerCall{0};
  /**
   * The inclusive runtime of the node based on cube info. This can be lower than the runtime of a child if the child
   * gets called by other functions too
   */
  double inclusiveRunTimeCube{0};
  /**
   * based on inclusiveRunTimeCube
   */
  double inclusiveTimePerCallCube{0};
  /**
   * The inclusive runtime of the node based on summing the runtime of all its childs and itself
   */
  double inclusiveRunTimeSum{0};
  /**
   * based on inclusiveRunTimeSum
   */
  double inclusiveTimePerCallSum{0};
  /**
   * True if all childs are instrumented
   */
  bool isExclusiveRuntime{false};

  /**
   * True if the node should be instrumented. Used to detect nodes with zero calls
   */
  bool shouldBeInstrumented{false};
};

}  // namespace pira

#endif

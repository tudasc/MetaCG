#include "CgNode.h"
#include "CallgraphManager.h"
#include "MCGManager.h"

#include "MetaDataHandler.h"

#include "spdlog/spdlog.h"

namespace metacg::io {
namespace retriever {

inline void to_json(json &j, const pira::BaseProfileData &data) {
  j = nlohmann::json{{"numCalls", data.getNumberOfCalls()},
                     {"timeInSeconds", data.getRuntimeInSeconds()},
                     {"inclusiveRtInSeconds", data.getInclusiveRuntimeInSeconds()}};
}

/**
 * This is the nlohmann::json way to serialize data and has been here before the MCGWriter existed.
 * @param j
 * @param data
 */
inline void to_json(json &j, const pira::PiraTwoData &data) {
  auto &gOpts = ::pgis::config::GlobalConfig::get();
  auto rtOnly = gOpts.getAs<bool>("runtime-only");

  auto rtAndParams = valTup(data.getRuntimeVec(), data.getExtrapParameters(), data.getNumReps());
  json experiments;
  for (auto elem : rtAndParams) {
    json exp{};
    exp["runtime"] = elem.first;
    exp[elem.second.first] = elem.second.second;
    experiments += exp;
  }
  if (!rtOnly) {
    j = json{{"experiments", experiments},
             {"model", data.getExtrapModel()->getAsString(data.getExtrapModelConnector().getParamList())}};
  } else {
    j = json{{"experiments", experiments}};
  }
  spdlog::get("console")->debug("PiraTwoData to_json:\n{}", j.dump());
}

json BaseProfileDataHandler::value(const CgNodePtr n) const {
  json j;
  to_json(j, *(n->get<pira::BaseProfileData>()));
  return j;
}

void BaseProfileDataHandler::read([[maybe_unused]] const json &j, const std::string &functionName) {
  spdlog::get("console")->trace("Running BaseProfileDataHandler::read");
  auto jIt = j[toolname];
  if (jIt.is_null()) {
    spdlog::get("console")->trace("Could not retrieve metadata for {} in function {}", toolname, functionName);
    return;
  }

  auto numCalls = jIt["numCalls"].get<unsigned long long int>();
  auto rtInSeconds = jIt["timeInSeconds"].get<double>();
  auto inclRtInSeconds = jIt["inclusiveRtInSeconds"].get<double>();
  auto node = mcgm->findOrCreateNode(functionName);
  auto bpd = getOrCreateMD<pira::BaseProfileData>(node);
  bpd->setNumberOfCalls(numCalls);
  bpd->setRuntimeInSeconds(rtInSeconds);
  bpd->setInclusiveRuntimeInSeconds(inclRtInSeconds);
  }

void PiraOneDataRetriever::read([[maybe_unused]] const json &j, const std::string &functionName) {
  spdlog::get("console")->trace("Running PiraOneMetaDataRetriever::read from json");
  auto jIt = j[toolname];
  if (jIt.is_null()) {
    spdlog::get("console")->trace("Could not retrieve meta data for {}", toolname);
    return;
  }
  auto numStmts = jIt.get<long long int>();
  spdlog::get("console")->debug("Read {} stmts from file", numStmts);
  auto node = mcgm->findOrCreateNode(functionName);
  auto pod = getOrCreateMD<pira::PiraOneData>(node);
  pod->setNumberOfStatements(numStmts);
}

bool PiraTwoDataRetriever::handles(const CgNodePtr n) const {
  spdlog::get("console")->trace("PiraTwoRetriever:handles {}", n->getFunctionName());
  // Keep the check and get semantics here
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

json PiraTwoDataRetriever::value(const CgNodePtr n) const {
  nlohmann::json j;
  to_json(j, *(n->get<pira::PiraTwoData>()));

  return j;
}

void PiraTwoDataRetriever::read([[maybe_unused]] const json &j, const std::string &functionName) {
  spdlog::get("console")->warn("Running PiraTwoDataRetriever::read from json, currently not implemented / supoprted");
}

void FilePropertyHandler::read(const json &j, const std::string &functionName) {
  auto jIt = j[toolname];
  if (jIt.is_null()) {
    spdlog::get("console")->trace("Could not retrieve meta data for {}", toolname);
  }
  std::string fileOrigin = jIt["origin"].get<std::string>();
  bool isFromSystemInclude = jIt["systemInclude"].get<bool>();
  auto node = mcgm->findOrCreateNode(functionName);
  auto md = getOrCreateMD<pira::FilePropertiesMetaData>(node);
  md->origin = fileOrigin;
  md->fromSystemInclude = isFromSystemInclude;
}

json FilePropertyHandler::value(const CgNodePtr n) const {
  json j;
  auto fpData = n->get<pira::FilePropertiesMetaData>();
  j["origin"] = fpData->origin;
  j["systemInclude"] = fpData->fromSystemInclude;
  return j;
}

void CodeStatisticsHandler::read(const json &j, const std::string &functionName) {
  auto jIt = j[toolname];
  if (jIt.is_null()) {
    spdlog::get("console")->trace("Could not retrieve meta data for {}", toolname);
  }
  int numVars = jIt["numVars"].get<int>();
  auto node = mcgm->findOrCreateNode(functionName);
  auto md = getOrCreateMD<pira::CodeStatisticsMetaData>(node);
  md->numVars = numVars;
}

json CodeStatisticsHandler::value(const CgNodePtr n) const {
  json j;
  auto csData = n->get<pira::CodeStatisticsMetaData>();
  j["numVars"] = csData->numVars;
  return j;
}

void NumOperationsHandler::read(const json &j, const std::string &functionName) {
  auto jIt = j[toolname];
  if (jIt.is_null()) {
    spdlog::get("console")->trace("Could not retrieve meta data for {}", toolname);
  }
  int numberOfIntOps = jIt["numberOfIntOps"].get<int>();
  int numberOfFloatOps = jIt["numberOfFloatOps"].get<int>();
  int numberOfControlFlowOps = jIt["numberOfControlFlowOps"].get<int>();
  int numberOfMemoryAccesses = jIt["numberOfMemoryAccesses"].get<int>();
  auto node = mcgm->findOrCreateNode(functionName);
  auto md = getOrCreateMD<pira::NumOperationsMetaData>(node);
  md->numberOfIntOps = numberOfIntOps;
  md->numberOfFloatOps = numberOfFloatOps;
  md->numberOfControlFlowOps = numberOfControlFlowOps;
  md->numberOfMemoryAccesses = numberOfMemoryAccesses;
}

json NumOperationsHandler::value(const CgNodePtr n) const {
  json j;
  auto noData = n->get<pira::NumOperationsMetaData>();
  j["numberOfIntOps"] = noData->numberOfIntOps;
  j["numberOfFloatOps"] = noData->numberOfFloatOps;
  j["numberOfControlFlowOps"] = noData->numberOfControlFlowOps;
  j["numberOfMemoryAccesses"] = noData->numberOfMemoryAccesses;
  return j;
}

void NumConditionalBranchHandler::read(const json &j, const std::string &functionName) {
  auto jIt = j[toolname];
  if (jIt.is_null()) {
    spdlog::get("console")->trace("Could not retrieve meta data for {}", toolname);
  }
  int numberOfConditionalBranches = jIt.get<int>();
  auto node = mcgm->findOrCreateNode(functionName);
  auto md = getOrCreateMD<pira::NumConditionalBranchMetaData>(node);
  md->numConditionalBranches = numberOfConditionalBranches;
}

json NumConditionalBranchHandler::value(const CgNodePtr n) const {
  json j;
  auto ncData = n->get<pira::NumConditionalBranchMetaData>();
  j["numConditionalBranches"] = ncData->numConditionalBranches;
  return j;
}

void LoopDepthHandler::read(const json &j, const std::string &functionName) {
  auto jIt = j[toolname];
  if (jIt.is_null()) {
    spdlog::get("console")->trace("Could not retrieve meta data for {}", toolname);
  }
  int loopDepth = jIt.get<int>();
  auto node = mcgm->findOrCreateNode(functionName);
  auto md = getOrCreateMD<pira::LoopDepthMetaData>(node);
  md->loopDepth = loopDepth;
}

json LoopDepthHandler::value(const CgNodePtr n) const {
  json j;
  auto ldData = n->get<pira::LoopDepthMetaData>();
  j["loopDepth"] = ldData->loopDepth;
  return j;
}

void GlobalLoopDepthHandler::read(const json &j, const std::string &functionName) {
  auto jIt = j[toolname];
  if (jIt.is_null()) {
    spdlog::get("console")->trace("Could not retrieve meta data for {}", toolname);
  }
  int globalLoopDepth = jIt.get<int>();
  auto node = mcgm->findOrCreateNode(functionName);
  auto md = getOrCreateMD<pira::GlobalLoopDepthMetaData>(node);
  md->globalLoopDepth = globalLoopDepth;
}

json GlobalLoopDepthHandler::value(const CgNodePtr n) const {
  json j;
  auto gldData = n->get<pira::GlobalLoopDepthMetaData>();
  j["globalLoopDepth"] = gldData->globalLoopDepth;
  return j;
}
}  // namespace retriever
}  // namespace metacg::io

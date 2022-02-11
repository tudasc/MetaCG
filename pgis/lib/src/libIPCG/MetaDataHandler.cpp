#include "../../../../graph/include/CgNode.h"
#include "CallgraphManager.h"
#include "MCGManager.h"

#include "MetaDataHandler.h"

#include "spdlog/spdlog.h"

namespace metacg {
namespace io {
namespace retriever {

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
  auto [has, md] = node->checkAndGet<pira::PiraOneData>();
  if (has) {
    md->setNumberOfStatements(numStmts);
  } else {
    assert(false && "Should already be attached.");
  }
}

bool PiraTwoDataRetriever::handles(const CgNodePtr n) const {
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

pira::PiraTwoData PiraTwoDataRetriever::value(const CgNodePtr n) const { return *(n->get<pira::PiraTwoData>()); }

void PiraTwoDataRetriever::read([[maybe_unused]] const json &j, const std::string &functionName) {
  spdlog::get("console")->trace("Running PiraTwoDataRetriever::read from json");
}

void FilePropertyHandler::read(const json &j, const std::string &functionName) {
  auto jIt = j[toolname];
  if (jIt.is_null()) {
    spdlog::get("console")->trace("Could not retrieve meta data for {}", toolname);
  }
  std::string fileOrigin = jIt["origin"].get<std::string>();
  bool isFromSystemInclude = jIt["systemInclude"].get<bool>();
  auto node = mcgm->findOrCreateNode(functionName);
  auto md = new pira::FilePropertiesMetaData();
  md->origin = fileOrigin;
  md->fromSystemInclude = isFromSystemInclude;
  node->addMetaData(md);
}

void CodeStatisticsHandler::read(const json &j, const std::string &functionName) {
  auto jIt = j[toolname];
  if (jIt.is_null()) {
    spdlog::get("console")->trace("Could not retrieve meta data for {}", toolname);
  }
  int numVars = jIt["numVars"].get<int>();
  auto node = mcgm->findOrCreateNode(functionName);
  auto md = new pira::CodeStatisticsMetaData();
  md->numVars = numVars;
  node->addMetaData(md);
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
  auto md = new pira::NumOperationsMetaData();
  md->numberOfIntOps = numberOfIntOps;
  md->numberOfFloatOps = numberOfFloatOps;
  md->numberOfControlFlowOps = numberOfControlFlowOps;
  md->numberOfMemoryAccesses = numberOfMemoryAccesses;
  node->addMetaData(md);
}
void NumConditionalBranchHandler::read(const json &j, const std::string &functionName) {
  auto jIt = j[toolname];
  if (jIt.is_null()) {
    spdlog::get("console")->trace("Could not retrieve meta data for {}", toolname);
  }
  int numberOfConditionalBranches = jIt.get<int>();
  auto node = mcgm->findOrCreateNode(functionName);
  auto md = new pira::NumConditionalBranchMetaData();
  md->numConditionalBranches = numberOfConditionalBranches;
  node->addMetaData(md);
}

void LoopDepthHandler::read(const json &j, const std::string &functionName) {
  auto jIt = j[toolname];
  if (jIt.is_null()) {
    spdlog::get("console")->trace("Could not retrieve meta data for {}", toolname);
  }
  int loopDepth = jIt.get<int>();
  auto node = mcgm->findOrCreateNode(functionName);
  auto md = new pira::LoopDepthMetaData();
  md->loopDepth = loopDepth;
  node->addMetaData(md);
}
void GlobalLoopDepthHandler::read(const json &j, const std::string &functionName) {
  auto jIt = j[toolname];
  if (jIt.is_null()) {
    spdlog::get("console")->trace("Could not retrieve meta data for {}", toolname);
  }
  int globalLoopDepth = jIt.get<int>();
  auto node = mcgm->findOrCreateNode(functionName);
  auto md = new pira::GlobalLoopDepthMetaData();
  md->globalLoopDepth = globalLoopDepth;
  node->addMetaData(md);
}
}  // namespace retriever
}  // namespace io
}  // namespace metacg

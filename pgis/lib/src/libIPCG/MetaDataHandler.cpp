#include "CallgraphManager.h"
#include "CgNode.h"

#include "libIPCG/MetaDataHandler.h"

#include "spdlog/spdlog.h"

namespace MetaCG {
namespace io {
namespace retriever {

void PiraOneDataRetriever::read([[maybe_unused]] const json &j, const std::string &functionName) {
  spdlog::get("console")->trace("Running PiraOneMetaDataRetriever::read from json");
  auto jIt = j[toolname];
  if (jIt.is_null()) {
    spdlog::get("console")->warn("Could not retrieve meta data for {}", toolname);
    return;
  }
  auto numStmts = jIt.get<long long int>();
  spdlog::get("console")->debug("Read {} stmts from file", numStmts);
  cgm->putNumberOfStatements(functionName, numStmts);
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
    spdlog::get("console")->warn("Could not retrieve meta data for {}", toolname);
  }
  std::string fileOrigin = jIt["origin"].get<std::string>();
  bool isFromSystemInclude = jIt["systemInclude"].get<bool>();
  auto node = cgm->findOrCreateNode(functionName);
  auto md = new pira::FilePropertiesMetaData();
  md->origin = fileOrigin;
  md->fromSystemInclude = isFromSystemInclude;
  node->addMetaData(md);
}

void CodeStatisticsHandler::read(const json &j, const std::string &functionName) {
  auto jIt = j[toolname];
  if (jIt.is_null()) {
    spdlog::get("console")->warn("Could not retrieve meta data for {}", toolname);
  }
  int numVars = jIt["numVars"].get<int>();
  auto node = cgm->findOrCreateNode(functionName);
  auto md = new pira::CodeStatisticsMetaData();
  md->numVars = numVars;
  node->addMetaData(md);
}

}  // namespace retriever
}  // namespace io
}  // namespace MetaCG

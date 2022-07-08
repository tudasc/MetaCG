/**
 * File: MetaDataHandler.h
 * License: Part of the metacg project. Licensed under BSD 3 clause license. See LICENSE.txt file at
 * https://github.com/tudasc/metacg/LICENSE.txt
 */

#ifndef METACG_LIBMCG_METADATA_HANDLER_H
#define METACG_LIBMCG_METADATA_HANDLER_H

// clang-format off
// Graph library
#include "CgNodePtr.h"

// PGIS library
#include "CgNodeMetaData.h"
#include "Utility.h"
#include "CallgraphManager.h"
#include "nlohmann/json.hpp"
#include "GlobalConfig.h"

// System library
#include <string>
#include <iostream>
// clang-format on

class PiraMCGProcessor;

namespace metacg {
namespace graph {
class MCGManager;
}
namespace io::retriever {

using json = nlohmann::json;

struct BaseProfileDataHandler : public MetaDataHandler {
  const std::string toolname{"baseProfileData"};
  bool handles(const CgNodePtr n) const override { return n->has<pira::BaseProfileData>(); }
  const std::string toolName() const override { return toolname; }
  json value(const CgNodePtr n) const override;
  void read([[maybe_unused]] const json &j, const std::string &functionName) override;
};

/**
 * Retrieves the number of statements per function in new metacg Reader design.
 */
struct PiraOneDataRetriever : public MetaDataHandler {
  const std::string toolname{"numStatements"};
  bool handles(const CgNodePtr n) const override { return n->has<pira::PiraOneData>(); }
  const std::string toolName() const override { return toolname; }
  json value(const CgNodePtr n) const override {
    json j;
    j["numStatements"] = n->get<pira::PiraOneData>()->getNumberOfStatements();
    return j;
  }
  void read([[maybe_unused]] const json &j, const std::string &functionName) override;
};

/**
 * Exports PIRA II data into json in new metacg Reader design
 */
struct PiraTwoDataRetriever : public MetaDataHandler {
  const std::string toolname{"PiraIIData"};
  bool handles(const CgNodePtr n) const override;
  json value(const CgNodePtr n) const override;
  const std::string toolName() const override { return toolname; }
  void read([[maybe_unused]] const json &j, const std::string &functionName) override;
};

/**
 * Reads file property meta data in metacg Reader design.
 */
struct FilePropertyHandler : public MetaDataHandler {
  const std::string toolname{"fileProperties"};
  bool handles(const CgNodePtr n) const override { return n->has<pira::FilePropertiesMetaData>(); }
  const std::string toolName() const override { return toolname; }
  void read(const json &j, const std::string &functionName) override;
  json value(const CgNodePtr n) const override;
};

struct CodeStatisticsHandler : public MetaDataHandler {
  const std::string toolname{"codeStatistics"};
  bool handles(const CgNodePtr n) const override { return n->has<pira::CodeStatisticsMetaData>(); }
  const std::string toolName() const override { return toolname; }
  void read(const json &j, const std::string &functionName) override;
  json value(const CgNodePtr n) const override;
};

struct NumConditionalBranchHandler : public MetaDataHandler {
  const std::string toolname{"numConditionalBranches"};
  void read(const json &j, const std::string &functionName) override;
  bool handles(const CgNodePtr n) const override { return n->has<pira::NumConditionalBranchMetaData>(); }
  const std::string toolName() const override { return toolname; }
  json value(const CgNodePtr n) const override;
};

struct NumOperationsHandler : public MetaDataHandler {
  const std::string toolname{"numOperations"};
  void read(const json &j, const std::string &functionName) override;
  bool handles(const CgNodePtr n) const override { return n->has<pira::NumOperationsMetaData>(); }
  const std::string toolName() const override { return toolname; }
  json value(const CgNodePtr n) const override;
};

struct LoopDepthHandler : public MetaDataHandler {
  const std::string toolname{"loopDepth"};
  void read(const json &j, const std::string &functionName) override;
  bool handles(const CgNodePtr n) const override { return n->has<pira::LoopDepthMetaData>(); }
  const std::string toolName() const override { return toolname; }
  json value(const CgNodePtr n) const override;
};

struct GlobalLoopDepthHandler : public MetaDataHandler {
  const std::string toolname{"globalLoopDepth"};
  void read(const json &j, const std::string &functionName) override;
  bool handles(const CgNodePtr n) const override { return n->has<pira::GlobalLoopDepthMetaData>(); }
  const std::string toolName() const override { return toolname; }
  json value(const CgNodePtr n) const override;
};

// namespace retriever
}  // namespace io::retriever
}  // namespace metacg
#endif

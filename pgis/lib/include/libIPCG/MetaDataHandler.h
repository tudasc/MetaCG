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

/**
 * This is to test, if it can actually work as imagined
 */
struct TestHandler : public MetaDataHandler {
  int i{0};
  const std::string toolName() const override { return "TestMetaHandler"; }
  void read([[maybe_unused]] const json &j, const std::string &functionName) override { i++; }
  bool handles(const CgNodePtr n) const override { return false; }
  int value(const CgNodePtr n) const { return i; }
};

/**
 * Retrieves the number of statements per function in new metacg Reader design.
 */
struct PiraOneDataRetriever : public MetaDataHandler {
  const std::string toolname{"numStatements"};
  bool handles(const CgNodePtr n) const override { return false; }
  const std::string toolName() const override { return toolname; }
  int value(const CgNodePtr n) const { return 42; }
  void read([[maybe_unused]] const json &j, const std::string &functionName) override;
};

/**
 * Exports PIRA II data into json in new metacg Reader design
 */
struct PiraTwoDataRetriever : public MetaDataHandler {
  const std::string toolname{"PiraIIData"};
  bool handles(const CgNodePtr n) const override;
  pira::PiraTwoData value(const CgNodePtr n) const;
  const std::string toolName() const override { return toolname; }
  void read([[maybe_unused]] const json &j, const std::string &functionName) override;
};

/**
 * Reads file property meta data in metacg Reader design.
 */
struct FilePropertyHandler : public MetaDataHandler {
  const std::string toolname{"fileProperties"};
  bool handles(const CgNodePtr n) const override { return true; }
  const std::string toolName() const override { return toolname; }
  void read(const json &j, const std::string &functionName) override;
};

struct CodeStatisticsHandler : public MetaDataHandler {
  const std::string toolname{"codeStatistics"};
  bool handles(const CgNodePtr n) const override { return false; }
  const std::string toolName() const override { return toolname; }
  void read(const json &j, const std::string &functionName) override;
};

struct NumConditionalBranchHandler : public MetaDataHandler {
  const std::string toolname{"numConditionalBranches"};
  void read(const json &j, const std::string &functionName) override;
  bool handles(const CgNodePtr n) const override { return false; }
  const std::string toolName() const override { return toolname; }
};

struct NumOperationsHandler : public MetaDataHandler {
  const std::string toolname{"numOperations"};
  void read(const json &j, const std::string &functionName) override;
  bool handles(const CgNodePtr n) const override { return false; }
  const std::string toolName() const override { return toolname; }
};

struct LoopDepthHandler : public MetaDataHandler {
  const std::string toolname{"loopDepth"};
  void read(const json &j, const std::string &functionName) override;
  bool handles(const CgNodePtr n) const override { return false; }
  const std::string toolName() const override { return toolname; }
};

struct GlobalLoopDepthHandler : public MetaDataHandler {
  const std::string toolname{"globalLoopDepth"};
  void read(const json &j, const std::string &functionName) override;
  bool handles(const CgNodePtr n) const override { return false; }
  const std::string toolName() const override { return toolname; }
};

  // namespace retriever
}  // namespace io
}  // namespace metacg
#endif

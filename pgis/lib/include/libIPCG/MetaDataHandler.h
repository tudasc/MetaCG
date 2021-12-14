/**
 * File: MetaDataHandler.h
 * License: Part of the MetaCG project. Licensed under BSD 3 clause license. See LICENSE.txt file at
 * https://github.com/tudasc/metacg/LICENSE.txt
 */

#ifndef METACG_LIBMCG_METADATA_HANDLER_H
#define METACG_LIBMCG_METADATA_HANDLER_H

#include "CgNodeMetaData.h"
#include "CgNodePtr.h"

#include "nlohmann/json.hpp"

#include <string>

#include <iostream>

class CallgraphManager;
namespace MetaCG {
namespace io {
namespace retriever {

using json = nlohmann::json;
/**
 * Example MetaRetriever
 *
 * A retriever needs to implement these three functions.
 */
struct MetaDataHandler {
  /** Invoked to decide if meta data should be output into the json file for the node */
  virtual bool handles(const CgNodePtr n) const = 0;

  /** Invoked to find or create meta data entry in json */
  virtual const std::string toolName() const = 0;

  /** Creates or returns the object to attach as meta information */
  const std::string value(const CgNodePtr n) const { return "Should not have happened"; }

  /** Reads the meta data from the json file and attaches it to the graph nodes */
  virtual void read([[maybe_unused]] const json &j, const std::string &functionName) = 0;

  /** Call back automatically invoked when adding to CallgraphManager */
  void registerCGManager(CallgraphManager *cgManager) { cgm = cgManager; }

  /** Reference to CallgraphManager (not owned) */
  CallgraphManager *cgm;
};

/**
 * This is to test, if it can actually work as imagined
 */
struct TestMetaHandler : public MetaDataHandler {
  bool handles(const CgNodePtr n) const override { return false; }
  virtual const std::string toolName() const override { return "TestMetaHandler"; }
  int value(const CgNodePtr n) const { return 42; }
  virtual void read([[maybe_unused]] const json &j, const std::string &functionName) override {}
};

/**
 * Retrieves the number of statements per function in new MetaCG Reader design.
 */
struct PiraOneDataRetriever : public MetaDataHandler {
  const std::string toolname{"numStatements"};
  bool handles(const CgNodePtr n) const override { return false; }
  virtual const std::string toolName() const override { return toolname; }
  int value(const CgNodePtr n) const { return 42; }
  virtual void read([[maybe_unused]] const json &j, const std::string &functionName) override;
};

/**
 * Exports PIRA II data into json in new MetaCG Reader design
 */
struct PiraTwoDataRetriever : public MetaDataHandler {
  const std::string toolname{"PiraIIData"};
  bool handles(const CgNodePtr n) const override;
  pira::PiraTwoData value(const CgNodePtr n) const;
  const std::string toolName() const override { return toolname; }
  void read([[maybe_unused]] const json &j, const std::string &functionName) override;
};

/**
 * Reads new file property meta data in MetaCG Reader deisgn.
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

}  // namespace retriever
}  // namespace io
}  // namespace MetaCG
#endif

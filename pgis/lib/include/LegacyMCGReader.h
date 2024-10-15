/**
 * File: LegacyMCGReader.h
 * License: Part of the metacg project. Licensed under BSD 3 clause license. See LICENSE.txt file at
 * https://github.com/tudasc/metacg/LICENSE.txt
 */

#ifndef METACG_PGIS_INCLUDE_LEGACYMCGREADER_H
#define METACG_PGIS_INCLUDE_LEGACYMCGREADER_H

#include "io/MCGReader.h"

namespace metacg::pgis::io {

/**
 * Class to read metacg files in file format v1.0.
 * The file format is also typically referred to as IPCG files.
 */
class VersionOneMetaCGReader : public metacg::io::MetaCGReader {
 public:
  explicit VersionOneMetaCGReader(metacg::io::ReaderSource& source) : MetaCGReader(source) {}

  /**
   * Internal data structure to create the metacg structure
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
    std::unordered_map<std::string, nlohmann::json> namedMetadata;
    // load imbalance detection data
    bool visited = false;
    bool irrelevant = false;
  };

 public:
  typedef std::unordered_map<std::string, FunctionInfo> FuncMapT;
  typedef std::unordered_map<std::string, std::unordered_set<std::string>> StrStrMap;

  std::unique_ptr<Callgraph> read() override;

 protected:
  /**
   * Gets or inserts new node into the functions LUT
   */
  FuncMapT::mapped_type& getOrInsert(const std::string& key);

  /**
   * Build the virtual function hierarchy in PGIS using the functions map
   */
  StrStrMap buildVirtualFunctionHierarchy(metacg::graph::MCGManager& cgManager);
  /**
   * Inserts all nodes in the function map into the final call-graph.
   * Uses potentialTargets to include all potential virtual call targets for virtual functions.
   */
  void buildGraph(metacg::graph::MCGManager& cgManager, StrStrMap& potentialTargets);

  /**
   * Checks if the jsonValue contains a value for key and sets field accordingly
   */
  template <typename FieldTy, typename JsonTy>
  void setIfNotNull(FieldTy& field, const JsonTy& jsonValue, const std::string& key) {
    auto jsonField = jsonValue.value()[key];
    if (!jsonField.is_null()) {
      field = jsonField.template get<typename std::remove_reference<FieldTy>::type>();
    } else {
      metacg::MCGLogger::instance().getErrConsole()->warn("Tried to read non-existing field {} for node.", key);
    }
  }

  /* Functions LUT: needed for construction of metacg */
  FuncMapT functions;

 private:
  void addNumStmts(metacg::graph::MCGManager& cgm);
};
}  // namespace metacg::pgis::io

#endif

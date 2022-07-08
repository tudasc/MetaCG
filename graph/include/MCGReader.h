/**
 * File: MCGReader.h
 * License: Part of the MetaCG project. Licensed under BSD 3 clause license. See LICENSE.txt file at
 * https://github.com/tudasc/metacg/LICENSE.txt
 */
#ifndef METACG_GRAPH_MCGREADER_H
#define METACG_GRAPH_MCGREADER_H

#include "MCGManager.h"
#include "LoggerUtil.h"

#include "nlohmann/json.hpp"

//#include <loadImbalance/LIMetaData.h>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <fstream>

namespace metacg::io {



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
  explicit FileSource(std::string filename) : filename(std::move(filename)) {}
  /**
   * Reads the json file with filename (provided at object construction)
   * and returns the json object.
   */
  virtual nlohmann::json get() const override {
    metacg::MCGLogger::instance().getConsole()->info("Reading metacg file from: {}", filename);
    nlohmann::json j;
    {
      std::ifstream in(filename);
      if (!in.is_open()) {
        metacg::MCGLogger::instance().getErrConsole()->error("Opening file {} failed.", filename);
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
  explicit JsonSource(nlohmann::json j) : json(j) {}
  virtual nlohmann::json get() const override { return json; }
  nlohmann::json json;
};

/**
 * Base class to read metacg files.
 *
 * Previously known as IPCG files, the metacg files are the serialized versions of the call graph.
 * This class implements basic functionality and is meant to be subclassed for different file versions.
 */
class MetaCGReader {
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
  explicit MetaCGReader(ReaderSource &src) : source(src) {}
  /**
   * PiraMCGProcessor object to be filled with the CG
   */
  virtual void read(metacg::graph::MCGManager &cgManager) = 0;

 protected:
  /**
   * Gets or inserts new node into the functions LUT
   */
  FuncMapT::mapped_type &getOrInsert(const std::string &key);

  /**
   * Checks if the jsonValue contains a value for key and sets field accordingly
   */
  template <typename FieldTy, typename JsonTy>
  void setIfNotNull(FieldTy &field, const JsonTy &jsonValue, const std::string &key) {
    auto jsonField = jsonValue.value()[key];
    if (!jsonField.is_null()) {
      field = jsonField.template get<typename std::remove_reference<FieldTy>::type>();
    } else {
      metacg::MCGLogger::instance().getErrConsole()->warn("Tried to read non-existing field {} for node.", key);
    }
  }

  /**
   * Build the virtual function hierarchy in PGIS using the functions map
   */
  StrStrMap buildVirtualFunctionHierarchy(metacg::graph::MCGManager &cgManager);

  /**
   * Inserts all nodes in the function map into the final call-graph.
   * Uses potentialTargets to include all potential virtual call targets for virtual functions.
   */
  void buildGraph(metacg::graph::MCGManager &cgManager, StrStrMap &potentialTargets);

  /* Functions LUT: needed for construction of metacg */
  FuncMapT functions;

  /**
   * Abstraction from where to read-in the JSON.
   */
  const ReaderSource &source;

 private:
  // filename of the metacg this instance parses
  const std::string filename;
};

/**
 * Class to read metacg files in file format v2.0.
 * The format contains the 'meta' field for tools to export information.
 */
class VersionTwoMetaCGReader : public MetaCGReader {
 public:
  explicit VersionTwoMetaCGReader(ReaderSource &source) : MetaCGReader(source) {}
  void read(metacg::graph::MCGManager &cgManager) override;
};

}  // namespace metacg::io

#endif

/**
 * File: MCGReader.h
 * License: Part of the MetaCG project. Licensed under BSD 3 clause license. See LICENSE.txt file at
 * https://github.com/tudasc/metacg/LICENSE.txt
 */
#ifndef METACG_GRAPH_MCGREADER_H
#define METACG_GRAPH_MCGREADER_H

#include "LoggerUtil.h"
#include "MCGManager.h"

#include "nlohmann/json.hpp"

#include <filesystem>
#include <fstream>
#include <string>
#include <unordered_map>
#include <unordered_set>

namespace metacg::io {

using json = nlohmann::json;

/**
 * Maps string identifiers used in the json file to the respective nodes in the internal call graph representation.
 */
struct StrToNodeMapping {
  virtual CgNode* getNodeFromStr(const std::string& formatId) = 0;
};

/**
 * Abstraction for the source of the JSON to read-in
 * Meant to be subclassed.
 */
struct ReaderSource {
  /**
   * Returns a json object of the call graph.
   */
  virtual nlohmann::json get() = 0;

  /**
   * Reads the format version string. May be overwritten by format specific readers.
   * @return The version string.
   */
  virtual std::string getFormatVersion() {
    auto j = get();
    if (!j.contains("_MetaCG") || !j.at("_MetaCG").contains("version")) {
      metacg::MCGLogger::instance().getErrConsole()->error("Unable to read version information from JSON source.");
      return "unknown";
    }
    return j.at("_MetaCG").at("version");
  }
};

/**
 * Wraps a file as the source of the JSON.
 * If the file does not exists, prints error and exits the program.
 */
struct FileSource : ReaderSource {
  explicit FileSource(std::filesystem::path filepath) : filepath(std::move(filepath)) {}
  /**
   * Reads the json file with filename (provided at object construction)
   * and returns the json object.
   */
  nlohmann::json get() override {
    if (!jsonContent.empty()) {
      return jsonContent;
    }

    const std::string filename = filepath.string();
    metacg::MCGLogger::instance().getConsole()->debug("Reading metacg file from: {}", filename);
    {
      std::ifstream in(filepath);
      if (!in.is_open()) {
        const std::string errorMsg = "Opening file " + filename + " failed.";
        metacg::MCGLogger::instance().getErrConsole()->error(errorMsg);
        throw std::runtime_error(errorMsg);
      }
      in >> jsonContent;
    }
    return jsonContent;
  };

  virtual ~FileSource() = default;

 private:
  std::filesystem::path filepath;
  nlohmann::json jsonContent;
};

/**
 * Wraps existing JSON object as source.
 * Currently only used in unit tests.
 */
struct JsonSource : ReaderSource {
  explicit JsonSource(nlohmann::json j) : json(std::move(j)) {}
  virtual ~JsonSource() = default;
  nlohmann::json get() override { return json; }

 private:
  nlohmann::json json;
};

/**
 * Base class to read metacg files.
 *
 * Previously known as IPCG files, the metacg files are the serialized versions of the call graph.
 * This class implements basic functionality and is meant to be subclassed for different file versions.
 */
class MetaCGReader {
 public:
  /**
   * filename path to file
   */
  explicit MetaCGReader(ReaderSource& src) : source(src) {}

  virtual ~MetaCGReader() = default;

  /**
   * PiraMCGProcessor object to be filled with the CG
   */
  [[nodiscard]] virtual std::unique_ptr<Callgraph> read() = 0;

 protected:
  /**
   * Abstraction from where to read-in the JSON.
   */
  ReaderSource& source;

 private:
  // filename of the metacg this instance parses
  const std::string filename;
};

/**
 * Factory function to instantiate the correct reader implementation for the given source.
 * @param src The source
 * @return A unique pointer to the instantiated reader. Empty, if there is no reader matching the format version.
 */
std::unique_ptr<MetaCGReader> createReader(ReaderSource& src);

}  // namespace metacg::io

#endif

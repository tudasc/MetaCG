
/**
 * File: VersionTwoMCGReader.cpp
 * License: Part of the MetaCG project. Licensed under BSD 3 clause license. See LICENSE.txt file at
 * https://github.com/tudasc/metacg/LICENSE.txt
 */
#include "io/VersionTwoMCGReader.h"
#include "io/VersionFourMCGReader.h"
#include "MCGBaseInfo.h"
#include "Timing.h"
#include "Util.h"

std::unique_ptr<metacg::Callgraph> metacg::io::VersionTwoMetaCGReader::read() {
  const metacg::RuntimeTimer rtt("VersionTwoMetaCGReader::read");
  const metacg::MCGFileFormatInfo ffInfo{2, 0};
  auto console = metacg::MCGLogger::instance().getConsole();
  auto errConsole = metacg::MCGLogger::instance().getErrConsole();

  auto j = source.get();

  if (j.is_null()) {
    const std::string errorMsg = "JSON source did not contain any data.";
    errConsole->error(errorMsg);
    throw std::runtime_error(errorMsg);
  }

  if (!j.contains(ffInfo.metaInfoFieldName) || j.at(ffInfo.metaInfoFieldName).is_null()) {
    const std::string errorMsg = "Could not read version info from metacg file.";
    errConsole->error(errorMsg);
    throw std::runtime_error(errorMsg);
  }

  auto& mcgInfo = j.at(ffInfo.metaInfoFieldName);
  // from here on we assume, that if any file meta information is given, it is correct

  /// XXX How to make that we can use the MCGGeneratorVersionInfo to access the identifiers
  auto mcgVersion = mcgInfo.at("version").get<std::string>();

  if (mcgVersion.compare(0, 1, std::string("2")) != 0) {
    const std::string errorMsg = "File is of version " + mcgVersion + ", this reader handles version 2.x";
    errConsole->error(errorMsg);
    throw std::runtime_error(errorMsg);
  }

  auto generatorName = mcgInfo.at("generator").at("name").get<std::string>();
  auto generatorVersion = mcgInfo.at("generator").at("version").get<std::string>();
  const MCGGeneratorVersionInfo genVersionInfo{generatorName, metacg::util::getMajorVersionFromString(generatorVersion),
                                               metacg::util::getMinorVersionFromString(generatorVersion), ""};
  console->info("The metacg (version {}) file was generated with {} (version: {})", mcgVersion, generatorName,
                generatorVersion);

  MCGFileInfo fileInfo{ffInfo, genVersionInfo};
  if (!j.contains(ffInfo.cgFieldName) || j.at(ffInfo.cgFieldName).is_null()) {
    const std::string errorMsg = "The call graph in the metacg file was not found or null.";
    errConsole->error(errorMsg);
    throw std::runtime_error(errorMsg);
  }

//  std::cout  << "Before: " << j << "\n"; // FIXME: remove

  auto& jsonCG = j[ffInfo.cgFieldName];
  console->info("Lifting MetaCG version {} file to version 4", mcgVersion);
  upgradeV2FormatToV4Format(jsonCG);
  mcgInfo["version"] = "4.0";

//  std::cout << "After: " << j << "\n"; // FIXME: remove

  JsonSource v4JsonSrc(j);
  VersionFourMetaCGReader v4Reader(v4JsonSrc);
  return v4Reader.read();
}

void metacg::io::VersionTwoMetaCGReader::upgradeV2FormatToV4Format(nlohmann::json& j) {

  // Iterate over all nodes
  for (auto& it : j.items()) {
    auto& jNode = it.value();

    // In V2, the function name is the identifier
    jNode["functionName"] = it.key();

    // Convert callees and calllers entries to edges
    jNode["edges"] = nlohmann::json::object();
    auto& jCallees = jNode["callees"];
    for (auto& jCallee: jCallees) {
      jNode["edges"][jCallee] = {};
    }
    jNode.erase("callees");
    jNode.erase("callers");

    // Create origin field by reading in fileProperties metadata
    auto& jMeta = jNode.at("meta");
    if (jMeta.contains("fileProperties") &&
        !jMeta.at("fileProperties").at("origin").get<std::string>().empty()) {
      jNode["origin"] = jMeta.at("fileProperties").at("origin");
      jMeta.at("fileProperties").erase("origin");
    } else {
      // if the V2 format did not contain origin data insert null
     jNode["origin"] = nullptr;
    }

    // Create OverrideMD if the function is virtual
    if (jNode.at("isVirtual")) {
      auto jOverrides = json::array();
      for (const auto& overrideNode : jNode.at("overrides")) {
        jOverrides.push_back(overrideNode);
      }
      auto jOverriddenBy = json::array();
      for (const auto& overridenByNode : jNode.at("overriddenBy")) {
        jOverriddenBy.push_back(overridenByNode);
      }
      jMeta["overrideMD"] = {{"overrides", jOverrides}, {"overriddenBy", jOverriddenBy}};

    }
    jNode.erase("isVirtual");
    jNode.erase("doesOverride");
    jNode.erase("overrides");
    jNode.erase("overriddenBy");
  }
}

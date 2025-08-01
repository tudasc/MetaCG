/**
 * File: CGFormat.cpp
 * License: Part of the MetaCG project. Licensed under BSD 3 clause license. See LICENSE.txt file at
 * https://github.com/tudasc/metacg/LICENSE.txt
 */

#include "config.h"

#include <iostream>
#include <fstream>
#include <string>

#include "LoggerUtil.h"
#include "io/MCGReader.h"
#include "io/MCGWriter.h"

#include "cxxopts.hpp"

using namespace metacg;

static std::optional<std::string> replacePathPrefix(const std::string& originalPath,
                                const std::string& targetDir,
                                const std::string& newPrefix) {
  std::string search = "/" + targetDir;
  size_t pos = originalPath.find(search);
  if (pos == std::string::npos) {
    return std::nullopt;
  }
  size_t afterTarget = pos + search.length();

  std::string new_path = newPrefix + originalPath.substr(afterTarget);
  return new_path;
}

int main(int argc, char** argv) {
  MCGLogger::logInfo("Running metacg::CGFormat (version {}.{})\nGit revision: {}", MetaCG_VERSION_MAJOR,
                     MetaCG_VERSION_MINOR, MetaCG_GIT_SHA);

  // clang-format off
  cxxopts::Options options("cgformat", "Formatting and canonicalization of MetaCG files");
  options.add_options()("input", "input MetaCG file", cxxopts::value<std::string>())(
      "o,output", "output file (default is to overwrite)", cxxopts::value<std::string>()->default_value(""))(
      "apply", "apply formatting and canonicalization", cxxopts::value<bool>()->default_value("false"))(
      "abort_after_error", "abort checking after the first error", cxxopts::value<bool>()->default_value("false"))(
      "origin_prefix", "check for a specific path prefix in the origin field", cxxopts::value<std::string>()->default_value(""))(
      "origin_prefix_to_replace", "marks the directory in the origin path that will be replaced by the prefix given in 'origin_prefix'", cxxopts::value<std::string>()->default_value(""))(
      "indent", "number of spaced used for indentation of the file. A value of -1 disables pretty-printing.", cxxopts::value<int>()->default_value("4"))(
      "discard_failed_metadata", "continue formatting, even if there is metadata that cannot be parsed and would be lost.", cxxopts::value<bool>()->default_value("false"))(
      "h,help", "Print help");
  // clang-format on

  options.parse_positional("input");

  const cxxopts::ParseResult result = options.parse(argc, argv);

  if (result.count("help")) {
    std::cout << options.help({""}) << std::endl;
    return EXIT_SUCCESS;
  }

  std::string inputFile = result["input"].as<std::string>();
  std::string outputFile = result["output"].as<std::string>();
  if (outputFile.empty()) {
    outputFile = inputFile;
  }
  bool overwriting = inputFile == outputFile;
  int indent = result["indent"].as<int>();
  std::string originPrefix = result["origin_prefix"].as<std::string>();
  std::string prefixToReplace = result["origin_prefix_to_replace"].as<std::string>();
  bool checkOnly = !result["apply"].as<bool>();
  bool discardFailedMd = result["discard_failed_metadata"].as<bool>();
  bool abortCheckAfterError = result["abort_after_error"].as<bool>();

  io::FileSource fs(inputFile);

  std::unordered_set<std::string> failedToRead;

  auto mcgReader = io::createReader(fs);
  mcgReader->onFailedMetadataRead([&failedToRead](NodeId, const std::string mdKey, nlohmann::json& mdVal) {
    failedToRead.insert(mdKey);
  });
  auto graph = mcgReader->read();

  bool checkSucceeded = true;

  // Checking for expected origin prefix
  if (!originPrefix.empty()) {
    for (auto& node : graph->getNodes()) {
      if (auto origin = node->getOrigin(); origin) {
        if (origin->empty()) {
          if (checkOnly) {
            MCGLogger::logError("Found empty origin field in node {} ('{}'). Use null instead.", node->getId(), node->getFunctionName());
            checkSucceeded = false;
            if (abortCheckAfterError) {
              return EXIT_FAILURE;
            }
          } else {
            node->setOrigin(std::nullopt);
          }
        } else {
          // This can be done with "starts_with" in C++20
          if (origin->size() < originPrefix.size() || origin->compare(0, originPrefix.size(), originPrefix) != 0) {
            if (checkOnly) {
              MCGLogger::logError("Origin field in node {} ('{}') does not match expected prefix. Value is: {}", node->getId(), node->getFunctionName(), *origin);
              checkSucceeded = false;
              if (abortCheckAfterError) {
                return EXIT_FAILURE;
              }
            } else if (!prefixToReplace.empty()) {
              auto newOrigin = replacePathPrefix(*origin, prefixToReplace, originPrefix);
              if (newOrigin) {
                node->setOrigin(newOrigin);
              } else {
                MCGLogger::logWarn("Unable to automatically replace prefix in origin path of node {} ('{}'): {}", node->getId(), node->getFunctionName(), *origin);
              }
            }
          }
        }
      }
    }
  }

  // If we're only running a check, we're done here.
  if (checkOnly) {
    return checkSucceeded ? EXIT_SUCCESS : EXIT_FAILURE;
  }

  // We abort if metadata would be lost, unless --discard_failed_metadata is set.
  // We always abort if the original file is to be overwritten.
  if (!failedToRead.empty()) {
    MCGLogger::logWarn("Some of the metadata entries could not be parsed.");

    if (!discardFailedMd) {
      MCGLogger::logError("Aborting export");
      return EXIT_FAILURE;
    }
    if (overwriting) {
      MCGLogger::logError("Please choose another output file to export with lost metadata");
      return EXIT_FAILURE;
    }
    MCGLogger::logWarn("Continuing with export - some of the metadata might be lost");
  }


  int outputMcgVersion = std::stoi(fs.getFormatVersion());

  auto mcgWriter = io::createWriter(outputMcgVersion);
  if (!mcgWriter) {
    MCGLogger::logError("Unable to create a writer for format version {}", outputMcgVersion);
    return EXIT_FAILURE;
  }

  io::JsonSink jsonSink;
  mcgWriter->write(graph.get(), jsonSink);

  std::ofstream os(outputFile);
  os << jsonSink.getJson().dump(indent) << std::endl;

  return EXIT_SUCCESS;
}

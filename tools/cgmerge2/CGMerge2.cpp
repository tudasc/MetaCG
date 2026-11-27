/**
 * File: CGMerge2.cpp
 * License: Part of the MetaCG project. Licensed under BSD 3 clause license. See LICENSE.txt file at
 * https://github.com/tudasc/metacg/LICENSE.txt
 */

#include "metacg/config.h"

#include <algorithm>
#include <cassert>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include <cxxopts.hpp>

#include "metacg/LoggerUtil.h"
#include "metacg/MCGManager.h"
#include "metacg/MergePolicy.h"
#include "metacg/io/MCGReader.h"
#include "metacg/io/MCGWriter.h"

// This may appear to be unused, but the variables declared here have side effects on the graph lib
#include "metacg/metadata/BuiltinMD.h"

using namespace metacg;

// The merge policy selected on the command line. Must be set before any merging happens.
static const MergePolicy* selectedMergePolicy = nullptr;

// Merges `source` into `target` using the selected merge policy.
// The OpenMP reduction combiner may only refer to omp_out/omp_in, so it accesses the policy through this function.
static void mergeWithSelectedPolicy(Callgraph& target, const Callgraph& source) {
  assert(selectedMergePolicy && "Merge policy must be set before merging");
  target.merge(source, *selectedMergePolicy);
}

#pragma omp declare reduction(cgmerge : std::unique_ptr<Callgraph> : mergeWithSelectedPolicy(*omp_out, *omp_in))       \
    initializer(omp_priv = std::make_unique<Callgraph>())

/**
 * Creates the merge policy with the given command-line name.
 * @return The policy, or nullptr if the name is unknown.
 */
static std::unique_ptr<MergePolicy> createMergePolicy(const std::string& name) {
  if (name == "name") {
    return std::make_unique<MergeByName>();
  }
  if (name == "name-and-origin") {
    return std::make_unique<MergeByNameAndOrigin>();
  }
  return nullptr;
}

int main(int argc, char** argv) {
  MCGLogger::logInfo("Running metacg::CGMerge2 (version {}.{})\nGit revision: {}", MetaCG_VERSION_MAJOR,
                     MetaCG_VERSION_MINOR, MetaCG_GIT_SHA);

  cxxopts::Options options("cgmerge2", "Merges multiple MetaCG call graphs into a single call graph.");
  options.positional_help("<outfile> <infile1> <infile2> ...");
  // clang-format off
  options.add_options()
    ("h,help", "Show this help message")
    ("p,merge-policy", "How nodes are matched during merging. 'name': nodes with the same function name are merged. "
                       "'name-and-origin': nodes are merged only if both function name and origin are the same.",
                       cxxopts::value<std::string>()->default_value("name"), "name|name-and-origin")
    ("log-level","The lowest severity of log messages to be emitted",cxxopts::value<std::string>(), "error|warning|info|debug|trace|off")
    ("output", "Output call graph file", cxxopts::value<std::string>())
    ("inputs", "Input call graph files", cxxopts::value<std::vector<std::string>>())
  ;
  // clang-format on

  options.parse_positional({"output", "inputs"});

  cxxopts::ParseResult result;
  try {
    result = options.parse(argc, argv);
  } catch (const cxxopts::exceptions::exception& e) {
    MCGLogger::logError("Invalid input arguments: {}", e.what());
    std::cout << options.help() << std::endl;
    return EXIT_FAILURE;
  }

  if (result.count("help")) {
    std::cout << options.help() << std::endl;
    return EXIT_SUCCESS;
  }

  spdlog::level::level_enum logLevel;
  if (result.count("log-level")) {
    if (result["log-level"].as<std::string>() == "error")
      logLevel = spdlog::level::err;
    else if (result["log-level"].as<std::string>() == "warning")
      logLevel = spdlog::level::warn;
    else if (result["log-level"].as<std::string>() == "info")
      logLevel = spdlog::level::info;
    else if (result["log-level"].as<std::string>() == "debug")
      logLevel = spdlog::level::debug;
    else if (result["log-level"].as<std::string>() == "trace")
      logLevel = spdlog::level::trace;
    else if (result["log-level"].as<std::string>() == "off")
      logLevel = spdlog::level::off;
    else {
      MCGLogger::logError("Log-level: {} not supported", result["log-level"].as<std::string>());
      return EXIT_FAILURE;
    }
  }

  metacg::MCGLogger::instance().getConsole()->set_level(logLevel);
  metacg::MCGLogger::instance().getErrConsole()->set_level(logLevel);

  if (!result.count("output") || !result.count("inputs")) {
    MCGLogger::logError("Invalid input arguments. Usage: cgmerge2 [options] <outfile> <infile1> <infile2> ...");
    std::cout << options.help() << std::endl;
    return EXIT_FAILURE;
  }

  const auto policyName = result["merge-policy"].as<std::string>();
  auto mergePolicy = createMergePolicy(policyName);
  if (!mergePolicy) {
    MCGLogger::logError("Unknown merge policy '{}'. Valid values are: name, name-and-origin", policyName);
    return EXIT_FAILURE;
  }
  selectedMergePolicy = mergePolicy.get();
  MCGLogger::logInfo("Using merge policy '{}'", policyName);

  const auto outfile = result["output"].as<std::string>();
  const auto inputFiles = result["inputs"].as<std::vector<std::string>>();

  // Detect format version
  int mcgVersion = 2;
  io::FileSource fs(inputFiles[0]);

  auto versionStr = fs.getFormatVersion();

  // Assuming here that the version string can be converted to int
  mcgVersion = std::stoi(fs.getFormatVersion());

  auto& mcgManager = graph::MCGManager::get();
  mcgManager.resetManager();

  // Every thread merges its input graphs into a private, initially empty graph; OpenMP then combines the private
  // graphs.
  auto mergedCg = std::make_unique<Callgraph>();

#pragma omp parallel for schedule(dynamic) reduction(cgmerge : mergedCg)
  for (auto& inFile : inputFiles) {
    io::FileSource fs(inFile);
    if (fs.get().is_null()) {
      MCGLogger::logError("Input file is NULL: {}", inFile);
      continue;
    }

    if (fs.getFormatVersion() != versionStr) {
      MCGLogger::logWarn("File format version does not match for input file {}", inFile);
    }

    auto mcgReader = io::createReader(fs);
    if (!mcgReader) {
      MCGLogger::logError("Unsupported MetaCG format version: {}", versionStr);
      exit(EXIT_FAILURE);
    }
    auto cg = mcgReader->read();
    mergeWithSelectedPolicy(*mergedCg, *cg);
  }

  mcgManager.addToManagedGraphs("merged", std::move(mergedCg), true);

  auto mcgWriter = io::createWriter(mcgVersion, getMetaCGGeneratorInfo("CGMerge2"));
  if (!mcgWriter) {
    MCGLogger::logError("Unable to create a writer for format version {}", mcgVersion);
    return EXIT_FAILURE;
  }

  io::JsonSink jsonSink;
  // Writes out the active graph
  mcgWriter->writeActiveGraph(jsonSink);

  std::ofstream os(outfile);
  os << jsonSink.getJson() << std::endl;

  MCGLogger::logInfo("Done merging");
  return EXIT_SUCCESS;
}
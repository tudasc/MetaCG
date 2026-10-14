/**
 * File: CGMerge2.cpp
 * License: Part of the MetaCG project. Licensed under BSD 3 clause license. See LICENSE.txt file at
 * https://github.com/tudasc/metacg/LICENSE.txt
 */

#include "metacg/config.h"

#include <algorithm>
#include <iostream>
#include <vector>

#include "metacg/LoggerUtil.h"
#include "metacg/MCGManager.h"
#include "metacg/io/MCGReader.h"
#include "metacg/io/VersionFourMCGReader.h"
#include "metacg/io/VersionFourMCGWriter.h"
#include "metacg/io/VersionTwoMCGReader.h"
#include "metacg/io/VersionTwoMCGWriter.h"

// This may appear to be unused, but the variables declared here have side effects on the graph lib
#include "metacg/metadata/BuiltinMD.h"

using namespace metacg;

int main(int argc, char** argv) {
  auto console = MCGLogger::instance().getConsole();
  auto errConsole = MCGLogger::instance().getErrConsole();

  console->info("Running metacg::CGMerge2 (version {}.{})\nGit revision: {}", MetaCG_VERSION_MAJOR,
                MetaCG_VERSION_MINOR, MetaCG_GIT_SHA);

  if (argc < 3) {
    errConsole->error("Invalid input arguments. Usage: cgmerge <outfile> <infile1> <infile2> ...");
    return EXIT_FAILURE;
  }

  const int outfilePos = 1;
  const int firstInputFilePos = outfilePos + 1;
  const int numInputFiles = argc - firstInputFilePos;

  const char* outfile = argv[outfilePos];

  std::vector<std::string> inputFiles;
  inputFiles.reserve(numInputFiles);
  for (int i = firstInputFilePos; i < argc; ++i) {
    inputFiles.emplace_back(argv[i]);
  }

  // Detect format version
  int mcgVersion = 2;
  io::FileSource fs(inputFiles[0]);

  auto versionStr = fs.getFormatVersion();

  // Assuming here that the version string can be converted to int
  mcgVersion = std::stoi(fs.getFormatVersion());

  auto& mcgManager = graph::MCGManager::get();
  mcgManager.resetManager();

  for (auto& inFile : inputFiles) {
    io::FileSource fs(inFile);
    if (fs.get().is_null()) {
      errConsole->error("Input file is NULL: {}", inFile);
      continue;
    }

    if (fs.getFormatVersion() != versionStr) {
      errConsole->warn("File format version does not match for input file {}", inFile);
    }

    auto mcgReader = io::createReader(fs);
    if (!mcgReader) {
      errConsole->error("Unsupported MetaCG format version: {}", versionStr);
      return EXIT_FAILURE;
    }
    mcgManager.addToManagedGraphs(inFile, mcgReader->read());
  }

  if (mcgManager.graphs_size() == 0) {
    errConsole->error("No valid input graphs to merge");
    return EXIT_FAILURE;
  }

  // TODO: Let user set merge policy
#ifdef _OPENMP
  // Get all graph pointers via all graph names
  auto nameSet = mcgManager.getAllManagedGraphNames();
  std::vector<std::string> graphNames(nameSet.begin(), nameSet.end());
  std::vector<Callgraph*> graphs;
  graphs.reserve(graphNames.size());
  for (const auto& name : graphNames) {
    graphs.push_back(mcgManager.getCallgraph(name));
  }

  // Binary-tree merge: at each level, graph i+stride is merged into i.
  const MergeByName policy;
  const long numGraphs = static_cast<long>(graphs.size());
  for (long stride = 1; stride < numGraphs; stride *= 2) {
    #pragma omp parallel for schedule(dynamic)
    for (long i = 0; i < numGraphs - stride; i += 2 * stride) {
      graphs[i]->merge(*graphs[i + stride], policy);
      graphs[i + stride]->clear();  // release memory of the absorbed graph early
    }
  }

  // The tree root holds the fully merged graph
  mcgManager.setActive(graphNames.front());
#else
  // Without OpenMP, sequentially merge all graphs into the active graph
  mcgManager.mergeAllIntoActiveGraph(MergeByName());
#endif


  auto mcgWriter = io::createWriter(mcgVersion);
  if (!mcgWriter) {
    errConsole->error("Unable to create a writer for format version {}", mcgVersion);
    return EXIT_FAILURE;
  }

  io::JsonSink jsonSink;
  // Writes out the active graph
  mcgWriter->writeActiveGraph(jsonSink);

  std::ofstream os(outfile);
  os << jsonSink.getJson() << std::endl;

  console->info("Done merging");
  return EXIT_SUCCESS;
}
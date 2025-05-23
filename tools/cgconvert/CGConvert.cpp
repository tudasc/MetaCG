/**
 * File: CGConvert.cpp
 * License: Part of the MetaCG project. Licensed under BSD 3 clause license. See LICENSE.txt file at
 * https://github.com/tudasc/metacg/LICENSE.txt
 */

#include "config.h"

#include <iostream>
#include <fstream>

#include "LoggerUtil.h"
#include "io/MCGReader.h"
#include "io/VersionTwoMCGWriter.h"

// This may appear to be unused, but the variables declared here have side effects on the graph lib
#include "metadata/BuiltinMD.h"

using namespace metacg;

int main(int argc, char** argv) {
  MCGLogger::logInfo("Running metacg::CGConvert (version {}.{})\nGit revision: {}", MetaCG_VERSION_MAJOR,
                     MetaCG_VERSION_MINOR, MetaCG_GIT_SHA);

  if (argc < 3) {
    MCGLogger::logError("Usage: {} <input_file> <output_file> [output_version]", argv[0]);
    return EXIT_FAILURE;
  }

  std::string inputFile = argv[1];
  std::string outputFile = argv[2];
  int outputMcgVersion = (argc >= 4) ? std::stoi(argv[3]) : 3;

  io::FileSource fs(inputFile);

  auto mcgReader = io::createReader(fs);
  auto graph = mcgReader->read();

  auto mcgWriter = io::createWriter(outputMcgVersion);
  if (!mcgWriter) {
    MCGLogger::logError("Unable to create a writer for format version {}", outputMcgVersion);
    return EXIT_FAILURE;
  }

  io::JsonSink jsonSink;
  mcgWriter->write(graph.get(), jsonSink);

  std::ofstream os(outputFile);
  os << jsonSink.getJson() << std::endl;

  return EXIT_SUCCESS;
}

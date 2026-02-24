/**
 * File: visual.cpp
 * License: Part of the MetaCG project. Licensed under BSD 3 clause license. See LICENSE.txt file at
 * https://github.com/tudasc/metacg/LICENSE.txt
 */

#include <Callgraph.h>
#include <DotIO.h>
#include <LoggerUtil.h>
#include <fstream>
#include <io/MCGReader.h>

static auto console = metacg::MCGLogger::instance().getConsole();
static auto errConsole = metacg::MCGLogger::instance().getErrConsole();

void printUsage() {
  std::cout << "Usage: visuel <cg>\n\n";
  std::cout << "Reads a call graph from the specified file and generates a DOT file for visualization.\n\n";
  std::cout << "Options:\n";
  std::cout << "  -h, --help      Show this help message\n";
  std::cout << "  -o, --output    Specify output DOT file name (default: callgraph.dot)\n";
}

int main(int argc, char* argv[]) {
  if (argc < 2) {
    printUsage();
    return EXIT_FAILURE;
  }

  std::string inputFile;
  std::string outputFile = "callgraph.dot";

  for (int i = 1; i < argc; i++) {
    std::string arg = argv[i];

    if (arg == "-h" || arg == "--help") {
      printUsage();
      return EXIT_SUCCESS;
    } else if (arg == "-o" || arg == "--output") {
      if (i + 1 >= argc) {
        errConsole->error("Output file name not specified after {}", arg);
        return EXIT_FAILURE;
      }
      outputFile = argv[++i];
    } else {
      inputFile = arg;
    }
  }

  if (inputFile.empty()) {
    errConsole->error("No input call graph file specified.");
    printUsage();
    return EXIT_FAILURE;
  }

  metacg::io::FileSource fs1(inputFile);

  auto mcgReader1 = metacg::io::createReader(fs1);
  if (!mcgReader1) {
    errConsole->error("Failed to create MCG reader for file: {}", inputFile);
    return EXIT_FAILURE;
  }

  auto cg = mcgReader1->read();
  if (!cg) {
    errConsole->error("Failed to read call graph from file.");
    return EXIT_FAILURE;
  }

  metacg::io::dot::DotGenerator dotGen(cg.get());
  dotGen.generate();

  std::ofstream outFile(outputFile);
  if (!outFile.is_open()) {
    errConsole->error("Could not open output file for writing: {}", outputFile);
    return EXIT_FAILURE;
  }
  outFile << dotGen.getDotString();

  outFile.close();

  console->info("DOT file generated successfully: {}", outputFile);

  return EXIT_SUCCESS;
}

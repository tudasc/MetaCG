/**
 * File: CGToDot.cpp
 * License: Part of the MetaCG project. Licensed under BSD 3 clause license. See LICENSE.txt file at
 * https://github.com/tudasc/metacg/LICENSE.txt
 */

#include "Callgraph.h"
#include "DotIO.h"
#include "LoggerUtil.h"
#include "io/MCGReader.h"

#include <cxxopts.hpp>
#include <fstream>

static auto console = metacg::MCGLogger::instance().getConsole();
static auto errConsole = metacg::MCGLogger::instance().getErrConsole();

int main(int argc, char* argv[]) {
  cxxopts::Options options("CGToDot",
                           "Reads a call graph from the specified file and generates a DOT file for visualization.");

  // clang-format off
  options.add_options()
    ("h,help", "Show this help message")
    ("o,output", "Specify output DOT file name", cxxopts::value<std::string>()->default_value("callgraph.dot"))
    ("input", "Input call graph file", cxxopts::value<std::string>())
  ;
  // clang-format on

  options.parse_positional({"input"});

  auto result = options.parse(argc, argv);

  if (result.count("help")) {
    std::cout << options.help() << std::endl;
    return EXIT_SUCCESS;
  }

  if (!result.count("input")) {
    errConsole->error("No input call graph file specified.");
    std::cout << options.help() << std::endl;
    return EXIT_FAILURE;
  }

  std::string inputFile = result["input"].as<std::string>();
  std::string outputFile = result["output"].as<std::string>();

  metacg::io::FileSource fs(inputFile);

  auto mcgReader = metacg::io::createReader(fs);
  if (!mcgReader) {
    errConsole->error("Failed to create MCG reader for file: {}", inputFile);
    return EXIT_FAILURE;
  }

  auto cg = mcgReader->read();
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

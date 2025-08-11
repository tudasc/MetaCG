/**
 * File: main.cpp
 * License: Part of the MetaCG project. Licensed under BSD 3 clause license. See LICENSE.txt file at
 * https://github.com/tudasc/metacg/LICENSE.txt
 */

#include "MCGManager.h"
#include "io/VersionTwoMCGReader.h"
#include "io/VersionTwoMCGWriter.h"
#include "LoggerUtil.h"

#include <fstream>
#include <string>

int main(int argc, char** argv) {
  std::string inFile(argv[1]);
  std::string outFile(argv[2]);

  metacg::MCGLogger::logInfo("Testing whether we can log {}!\n", "something");

  auto& manager = metacg::graph::MCGManager::get();
  metacg::io::FileSource fSource(inFile);
  metacg::io::VersionTwoMetaCGReader reader(fSource);
  manager.addToManagedGraphs("testG", reader.read());

  metacg::io::VersionTwoMCGWriter writer;
  metacg::io::JsonSink js;
  writer.writeActiveGraph(js);
  {
    std::ofstream os(outFile);
    js.output(os);
  }

  return 0;
}

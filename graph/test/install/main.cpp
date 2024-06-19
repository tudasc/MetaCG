/**
 * File: main.cpp
 * License: Part of the MetaCG project. Licensed under BSD 3 clause license. See LICENSE.txt file at
 * https://github.com/tudasc/metacg/LICENSE.txt
 */

#include "Callgraph.h"
#include "io/VersionTwoMCGReader.h"
#include "MCGManager.h"
#include "io/VersionTwoMCGWriter.h"

#include <string>
#include <fstream>

int main(int argc, char **argv) {

  std::string inFile(argv[1]);
  std::string outFile(argv[2]);

  auto &manager = metacg::graph::MCGManager::get();
  auto *graph = manager.getOrCreateCallgraph("testG", true);
  metacg::io::FileSource fSource(inFile);
  metacg::io::VersionTwoMetaCGReader reader(fSource);
  reader.read(manager);

  metacg::io::VersionTwoMCGWriter writer(manager);
  metacg::io::JsonSink js;
  writer.write(js);
  {
    std::ofstream os(outFile);
    js.output(os);
  }

  return 0;
}

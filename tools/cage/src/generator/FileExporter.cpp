/**
 * File: FileExporter.cpp
 * License: Part of the MetaCG project. Licensed under BSD 3 clause license. See LICENSE.txt file at
 * https://github.com/tudasc/metacg/LICENSE.txt
 */
#include "cage/generator/FileExporter.h"

#include "llvm/Support/raw_ostream.h"

#include "config.h"
#include "io/MCGWriter.h"
#include "io/VersionFourMCGWriter.h"

#include <fstream>

namespace cage {

void FileExporter::consumeCallGraph(metacg::Callgraph& graph) {
  metacg::io::JsonSink jsSink;
  metacg::io::VersionFourMCGWriter mcgw({{4, 0}, {"CaGe", 0, 1, MetaCG_GIT_SHA}}, true, true);
  mcgw.write(&graph, jsSink);

  llvm::outs() << "Writing generated call graph to: " << outfile << "\n";
  std::ofstream out(outfile);
  out << jsSink.getJson().dump(4);
  out.flush();
  out.close();
}

}  // namespace cage

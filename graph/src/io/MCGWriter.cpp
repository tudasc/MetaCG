/**
 * File: MCGWriter.cpp
 * License: Part of the MetaCG project. Licensed under BSD 3 clause license. See LICENSE.txt file at
 * https://github.com/tudasc/metacg/LICENSE.txt
 */

#include "io/MCGWriter.h"
#include "LoggerUtil.h"
#include "MCGManager.h"

void metacg::io::MCGWriter::writeActiveGraph(metacg::io::JsonSink& js) {
  const auto* cg = metacg::graph::MCGManager::get().getCallgraph();
  if (!cg) {
    MCGLogger::instance().getErrConsole()->error("Unable to write active graph: graph does not exist");
    return;
  }
  write(cg, js);
}

void metacg::io::MCGWriter::writeNamedGraph(const std::string& cgName, metacg::io::JsonSink& js) {
  const auto* cg = metacg::graph::MCGManager::get().getCallgraph(cgName);
  if (!cg) {
    MCGLogger::instance().getErrConsole()->error("Unable to write graph with name {}: graph does not exist", cgName);
    return;
  }
  write(cg, js);
}
/**
 * File: EstimatorPhase.cpp
 * License: Part of the metacg project. Licensed under BSD 3 clause license. See LICENSE.txt file at
 * https://github.com/tudasc/metacg/LICENSE.txt
 */

#include "EstimatorPhase.h"
#include "MetaData/CgNodeMetaData.h"
#include "MetaData/PGISMetaData.h"
#include <fstream>
#include <iomanip>  //  std::setw()
#include <iostream>

using namespace metacg;
using namespace pira;

#define NO_DEBUG

EstimatorPhase::EstimatorPhase(std::string name, metacg::Callgraph *callgraph)
    : graph(callgraph), IC(), name(std::move(name)), config(nullptr), noReportRequired(false) {}

void EstimatorPhase::generateIC() {
  for (const auto& elem : graph->getNodes()) {
    const auto& node= elem.second.get();
    if (pgis::isInstrumented(node)) {
      IC.instrumentedNames.insert(node->getFunctionName());
      IC.instrumentedNodes.insert(node);
    } else if (pgis::isInstrumentedPath(node)) {
      IC.instrumentedPaths.insert({node->getFunctionName(), node});
    }
  }
}

InstrumentationConfiguration EstimatorPhase::getIC() { return IC; }

void EstimatorPhase::printReport() {}

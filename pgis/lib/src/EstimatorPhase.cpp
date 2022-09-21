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

EstimatorPhase::EstimatorPhase(std::string name, bool isMetaPhase)
    : graph(nullptr),  // just so eclipse does not nag
      report(),        // initializes all members of report
      name(std::move(name)),
      config(nullptr),
      noReportRequired(isMetaPhase) {}

void EstimatorPhase::generateReport() {
  for (const auto node : *graph) {
    if (pgis::isInstrumented(node)) {
      report.instrumentedNames.insert(node->getFunctionName());
      report.instrumentedNodes.insert(node);
    } else if (pgis::isInstrumentedPath(node)) {
      report.instrumentedPaths.insert({node->getFunctionName(), node});
    }

    // Edge instrumentation
    // XXX This is used in LI implementation
    for (CgNodePtr parent : node->getInstrumentedParentEdges()) {
      report.instrumentedEdges.insert({parent, node});
    }
  }
}

void EstimatorPhase::setGraph(Callgraph *graph) { this->graph = graph; }

CgReport EstimatorPhase::getReport() { return this->report; }

void EstimatorPhase::printReport() {
  if (noReportRequired) {
    return;
  }
  if (config->tinyReport) {
    if (!report.metaPhase) {
      double overallOvPercent = report.instrOvPercent + report.unwindOvPercent + report.samplingOvPercent;
      std::cout << "==" << report.phaseName << "==  " << overallOvPercent << " %" << std::endl;
    }
  } else {
    std::cout << "==" << report.phaseName << "==  " << std::endl;
    if (!report.metaPhase) {
      printAdditionalReport();
    }
  }
}

void EstimatorPhase::printAdditionalReport() {
  if (report.instrumentedCalls > 0) {
    std::cout << " INSTR \t" << std::setw(8) << std::left << report.instrOvPercent << " %"
              << " | instr. " << report.instrumentedMethods << " of " << report.overallMethods << " methods"
              << " | instrCalls: " << report.instrumentedCalls << " | instrOverhead: " << report.instrOvSeconds << " s"
              << std::endl;
  }
  if (report.unwindSamples > 0) {
    std::cout << "   UNW \t" << std::setw(8) << report.unwindOvPercent << " %"
              << " | unwound " << report.unwConjunctions << " of " << report.overallConjunctions << " conj."
              << " | unwindSamples: " << report.unwindSamples << " | undwindOverhead: " << report.unwindOvSeconds
              << " s" << std::endl;
  }
  if (!config->ignoreSamplingOv) {
    std::cout << " SAMPL \t" << std::setw(8) << report.samplingOvPercent << " %"
              << " | taken samples: " << report.samplesTaken << " | samplingOverhead: " << report.samplingOvSeconds
              << " s" << std::endl;
  }
  std::cout << " ---->\t" << std::setw(8) << report.overallPercent << " %"
            << " | overallOverhead: " << report.overallSeconds << " s" << std::endl;
}


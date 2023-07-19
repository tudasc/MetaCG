/**
 * File: PiraMCGProcessor.cpp
 * License: Part of the metacg project. Licensed under BSD 3 clause license. See LICENSE.txt file at
 * https://github.com/tudasc/metacg/LICENSE.txt
 */

#include "ErrorCodes.h"
#include "PiraMCGProcessor.h"
#include "Timing.h"
#include "config/GlobalConfig.h"
#include "config/ParameterConfig.h"
#include "DotIO.h"
#include "ExtrapConnection.h"

#include "EXTRAP_Model.hpp"

#include "loadImbalance/LIMetaData.h"

#include "spdlog/spdlog.h"

#include <iomanip>  //  std::setw()

using namespace metacg;
using namespace pira;
using namespace ::pgis::options;

metacg::pgis::PiraMCGProcessor::PiraMCGProcessor(Config *config, extrapconnection::ExtrapConfig epCfg)
    : graph(&getEmptyGraph()), configPtr(config), epModelProvider(epCfg) {}

void metacg::pgis::PiraMCGProcessor::registerEstimatorPhase(EstimatorPhase *phase, bool noReport) {
  phases.push(phase);
  phase->injectConfig(configPtr);

  if (noReport) {
    phase->setNoReport();
  }
}

void metacg::pgis::PiraMCGProcessor::finalizeGraph(bool buildMarker) {
  if (graph->isEmpty()) {
    spdlog::get("errconsole")->error("Running the processor on empty graph. Need to construct graph.");
    exit(::pgis::ErrorCode::NoGraphConstructed);
  }

  // XXX We should double-check if this is still required, with the reachability now
  // being part of an explicit analysis class.
  if (graph->size() > 0) {
    // We assume that 'main' is always reachable.
    auto mainNode = graph->getMain();
    if (mainNode == nullptr) {
      spdlog::get("errconsole")->error("PiraMCGProcessor: Cannot find main function");
      exit(::pgis::ErrorCode::NoMainFunctionFound);
    }
  }
}

void metacg::pgis::PiraMCGProcessor::applyRegisteredPhases() {
  finalizeGraph();
  auto mainFunction = graph->getMain();

  if (mainFunction == nullptr) {
    spdlog::get("errconsole")->error("PiraMCGProcessor: Cannot find main function.");
    exit(::pgis::ErrorCode::NoMainFunctionFound);
  }

  while (!phases.empty()) {
    EstimatorPhase *phase = phases.front();

    {  // RAII
      const std::string curPhase = phase->getName();
      metacg::RuntimeTimer rtt("Running Prerequisites for " + curPhase);
      phase->doPrerequisites();
    }

    {  // RAII
      const std::string curPhase = phase->getName();
      metacg::RuntimeTimer rtt("Running " + curPhase);
      phase->modifyGraph(mainFunction);
      phase->generateIC();

      spdlog::get("console")->info("Print phase report");
      phase->printReport();

      InstrumentationConfiguration IC = phase->getIC();
      [[maybe_unused]] auto &gOpts = ::pgis::config::GlobalConfig::get();

      dumpInstrumentedNames(IC);  // outputs the instrumentation

    }  // RAII

    if (outputDotBetweenPhases) {
      metacg::io::dot::DotGenerator dotGenerator(graph);
      dotGenerator.generate();
      dotGenerator.output({"./DotOutput", "PGIS-Dot", phase->getName()});
    }

    phases.pop();
    // We don't know if estimator phases hold references / pointers to other EstimatorPhases
    // donePhases.push_back(std::shared_ptr<EstimatorPhase>{phase});
  }
}

int metacg::pgis::PiraMCGProcessor::getNumProcs() {
  int numProcs = 1;
  int prevNum = 0;
  for (const auto& elem : graph->getNodes()) {
    const auto& node= elem.second.get();
    if (!node->get<BaseProfileData>()->getCgLocation().empty()) {
      for (CgLocation cgLoc : node->get<BaseProfileData>()->getCgLocation()) {
        if (cgLoc.getProcId() != prevNum) {
          prevNum = cgLoc.getProcId();
          numProcs++;
        }
      }
    }
    if (numProcs > 1) {
      break;
    }
  }
  return numProcs;
}

bool metacg::pgis::PiraMCGProcessor::readWhitelist(std::vector<std::string> &whiteNodes) {
  std::ifstream in(configPtr->whitelist.c_str());

  if (!in) {
    spdlog::get("errconsole")->error("Cannot open file {}", configPtr->whitelist);
    return false;
  }

  std::string str;
  while (std::getline(in, str)) {
    if (str.empty()) {
      continue;
    }
    whiteNodes.push_back(str);
  }
  in.close();
  return true;
}

bool metacg::pgis::PiraMCGProcessor::isNodeListed(std::vector<std::string> whiteNodes, std::string node) {
  for (auto wNode : whiteNodes) {
    if (node == wNode) {
      return true;
    }
  }
  return false;
}

void metacg::pgis::PiraMCGProcessor::dumpInstrumentedNames(InstrumentationConfiguration IC) {
  if (noOutputRequired) {
    return;
  }

  std::string filename = configPtr->outputFile + "/instrumented-" + configPtr->appName + "-" + IC.phaseName + ".txt";
  std::size_t found = filename.find(configPtr->outputFile + "/instrumented-" + configPtr->appName + "-" + "Incl");
  if (found != std::string::npos) {
    filename = configPtr->outputFile + "/instrumented-" + configPtr->appName + ".txt";
  }
  filename = configPtr->outputFile + "/instrumented-" + configPtr->appName + ".txt";
  spdlog::get("console")->info("Writing to {}", filename);
  std::ofstream outfile(filename, std::ofstream::out);

  // The simple whitelist used so far in PIRA
  bool scorepOutput = ::pgis::config::GlobalConfig::get().getAs<bool>("scorep-out");
  if (!scorepOutput) {
    spdlog::get("console")->debug("Using plain whitelist format");
    if (IC.instrumentedNodes.empty()) {
      outfile << "aFunctionThatDoesNotExist" << std::endl;
    } else {
      for (const auto &name : IC.instrumentedNames) {
        outfile << name << std::endl;
      }
    }
  } else {
    spdlog::get("console")->debug("Using score-p format");
    const std::string scorepBegin{"SCOREP_REGION_NAMES_BEGIN"};
    const std::string scorepEnd{"SCOREP_REGION_NAMES_END"};
    const std::string mangled{"MANGLED"};
    const std::string arrow{"->"};
    const std::string include{"INCLUDE"};

    std::stringstream ss;
    ss << scorepBegin << "\n";
    for (const auto &name : IC.instrumentedNames) {
      ss << include << " " << name << "\n";
    }
    for (const auto &[name, node] : IC.instrumentedPaths) {
      for (const auto &parent : graph->getCallers(node)) {
        ss << include << " " << parent->getFunctionName() << " " << arrow << " " << name << "\n";
      }
    }

    ss << scorepEnd << "\n";

    outfile << ss.str();
  }
}

Callgraph* metacg::pgis::PiraMCGProcessor::getCallgraph(PiraMCGProcessor *cg) { if (cg) { return cg->graph; } return graph; }

void metacg::pgis::PiraMCGProcessor::attachExtrapModels() {
  epModelProvider.buildModels();
  for (const auto &elem : graph->getNodes()) {
    const auto& n=elem.second.get();
    spdlog::get("console")->debug("Attaching models for {}", n->getFunctionName());
    auto ptd = n->getOrCreateMD<PiraTwoData>(epModelProvider.getModelFor(n->getFunctionName()));
    if (!ptd->getExtrapModelConnector().hasModels()) {
      spdlog::get("console")->trace("attachExtrapModels hasModels == false -> Setting new ModelConnector");
      ptd->setExtrapModelConnector(epModelProvider.getModelFor(n->getFunctionName()));
    }

    ptd->getExtrapModelConnector().setEpolator(extrapconnection::ExtrapExtrapolator(epModelProvider.getConfigValues()));

    if (ptd->getExtrapModelConnector().hasModels()) {
      spdlog::get("console")->trace("attachExtrapModels for {} hasModels == true -> Use model aggregation strategy.",
                                    n->getFunctionName());
      auto &pConfig = ::pgis::config::ParameterConfig::get();
      ptd->getExtrapModelConnector().modelAggregation(pConfig.getPiraIIConfig()->modelAggregationStrategy);
    }
    spdlog::get("console")->debug("{}: No. of models: {}, model is set {}", n->getFunctionName(),
                                  n->get<PiraTwoData>()->getExtrapModelConnector().modelCount(),
                                  n->get<PiraTwoData>()->getExtrapModelConnector().isModelSet());
    spdlog::get("console")->debug("{}: models: {}", n->getFunctionName(),
                                  n->get<PiraTwoData>()->getExtrapModelConnector().getModelStrings());
    if (n->get<PiraTwoData>()->getExtrapModelConnector().isModelSet()) {
    }
    spdlog::get("console")->debug("{}: aggregated/selected model: {}", n->getFunctionName(),
                                  n->get<PiraTwoData>()->getExtrapModelConnector().getEPModelFunctionAsString());
  }
  spdlog::get("console")->info("Attaching Extra-P models done");
}

/**
 * File: PiraMCGProcessor.cpp
 * License: Part of the metacg project. Licensed under BSD 3 clause license. See LICENSE.txt file at
 * https://github.com/tudasc/metacg/LICENSE.txt
 */

#include "PiraMCGProcessor.h"
#include "DotIO.h"
#include "ErrorCodes.h"
#include "ExtrapConnection.h"
#include "Timing.h"
#include "config/GlobalConfig.h"
#include "config/ParameterConfig.h"

#include "EXTRAP_Model.hpp"

#include "loadImbalance/LIMetaData.h"

#include <climits>
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
  auto errConsole = metacg::MCGLogger::instance().getErrConsole();
  if (graph->isEmpty()) {
    errConsole->error("Running the processor on empty graph. Need to construct graph.");
    exit(metacg::pgis::ErrorCode::NoGraphConstructed);
  }

  // XXX We should double-check if this is still required, with the reachability now
  // being part of an explicit analysis class.
  if (graph->size() > 0) {
    // We assume that 'main' is always reachable.
    auto mainNode = graph->getMain();
    if (mainNode == nullptr) {
      errConsole->error("PiraMCGProcessor: Cannot find main function");
      exit(metacg::pgis::ErrorCode::NoMainFunctionFound);
    }
  }
}

void metacg::pgis::PiraMCGProcessor::applyRegisteredPhases() {
  finalizeGraph();
  auto mainFunction = graph->getMain();

  if (mainFunction == nullptr) {
    metacg::MCGLogger::instance().getErrConsole()->error("PiraMCGProcessor: Cannot find main function.");
    exit(metacg::pgis::ErrorCode::NoMainFunctionFound);
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

      metacg::MCGLogger::instance().getConsole()->info("Print phase report");
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
  for (const auto &elem : graph->getNodes()) {
    const auto &node = elem.second.get();
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
    metacg::MCGLogger::instance().getErrConsole()->error("Cannot open file {}", configPtr->whitelist);
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
  auto console = metacg::MCGLogger::instance().getConsole();

  //  std::string filename =
  //      configPtr->outputFile + "/instrumented-" + configPtr->appName + "-" + IC.phaseName + ".txt";
  //  std::size_t found = filename.find(configPtr->outputFile + "/instrumented-" + configPtr->appName + "-" + "Incl");
  //  if (found != std::string::npos) {
  //    filename = configPtr->outputFile + "/instrumented-" + configPtr->appName + ".txt";
  //  }
  std::string filename = configPtr->outputFile + "/instrumented-" + configPtr->appName + ".txt";
  char buff[PATH_MAX];
  const auto ret = getcwd(buff, PATH_MAX);
  if (ret == nullptr) {
    metacg::MCGLogger::instance().getErrConsole()->error("Cannot get current working directory");
    exit(metacg::pgis::ErrorCode::CouldNotGetCWD);
  }
  std::string curCwd(buff);
  console->info("Writing to {}. Current cwd {}", filename, curCwd);
  std::ofstream outfile(filename, std::ofstream::out);

  // The simple whitelist used so far in PIRA
  bool scorepOutput = ::pgis::config::GlobalConfig::get().getAs<bool>("scorep-out");
  if (!scorepOutput) {
    console->debug("Using plain whitelist format");
    if (IC.instrumentedNodes.empty()) {
      outfile << "aFunctionThatDoesNotExist" << std::endl;
    } else {
      for (const auto &name : IC.instrumentedNames) {
        outfile << name << std::endl;
      }
    }
  } else {
    console->debug("Using score-p format");
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

Callgraph *metacg::pgis::PiraMCGProcessor::getCallgraph(PiraMCGProcessor *cg) {
  if (cg) {
    return cg->graph;
  }
  return graph;
}

void metacg::pgis::PiraMCGProcessor::attachExtrapModels() {
  auto console = metacg::MCGLogger::instance().getConsole();
  epModelProvider.buildModels();
  for (const auto &elem : graph->getNodes()) {
    const auto &n = elem.second.get();
    console->debug("Attaching models for {}", n->getFunctionName());
    auto ptd = n->getOrCreateMD<PiraTwoData>(epModelProvider.getModelFor(n->getFunctionName()));
    if (!ptd->getExtrapModelConnector().hasModels()) {
      console->trace("attachExtrapModels hasModels == false -> Setting new ModelConnector");
      ptd->setExtrapModelConnector(epModelProvider.getModelFor(n->getFunctionName()));
    }

    ptd->getExtrapModelConnector().setEpolator(extrapconnection::ExtrapExtrapolator(epModelProvider.getConfigValues()));

    if (ptd->getExtrapModelConnector().hasModels()) {
      console->trace("attachExtrapModels for {} hasModels == true -> Use model aggregation strategy.",
                     n->getFunctionName());
      auto &pConfig = ::pgis::config::ParameterConfig::get();
      ptd->getExtrapModelConnector().modelAggregation(pConfig.getPiraIIConfig()->modelAggregationStrategy);
    }
    console->debug("{}: No. of models: {}, model is set {}", n->getFunctionName(),
                   n->get<PiraTwoData>()->getExtrapModelConnector().modelCount(),
                   n->get<PiraTwoData>()->getExtrapModelConnector().isModelSet());
    console->debug("{}: models: {}", n->getFunctionName(),
                   n->get<PiraTwoData>()->getExtrapModelConnector().getModelStrings());
    if (n->get<PiraTwoData>()->getExtrapModelConnector().isModelSet()) {
    }
    console->debug("{}: aggregated/selected model: {}", n->getFunctionName(),
                   n->get<PiraTwoData>()->getExtrapModelConnector().getEPModelFunctionAsString());
  }
  console->info("Attaching Extra-P models done");
}

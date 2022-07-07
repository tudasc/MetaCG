/**
 * File: PiraMCGProcessor.cpp
 * License: Part of the metacg project. Licensed under BSD 3 clause license. See LICENSE.txt file at
 * https://github.com/tudasc/metacg/LICENSE.txt
 */

#include "CallgraphManager.h"
#include "Timing.h"
#include "config/GlobalConfig.h"
#include "config/ParameterConfig.h"

#include "ExtrapConnection.h"

#include "EXTRAP_Model.hpp"

#include "loadImbalance/LIMetaData.h"

#include "spdlog/spdlog.h"

#include <iomanip>  //  std::setw()

using namespace metacg;
using namespace pira;
using namespace ::pgis::options;

metacg::pgis::PiraMCGProcessor::PiraMCGProcessor(Config *config, extrapconnection::ExtrapConfig epCfg)
    : graph(getEmptyGraph()), configPtr(config), epModelProvider(epCfg) {}

void metacg::pgis::PiraMCGProcessor::registerEstimatorPhase(EstimatorPhase *phase, bool noReport) {
  phases.push(phase);
  phase->injectConfig(configPtr);
  phase->setGraph(&graph);

  if (noReport) {
    phase->setNoReport();
  }
}

void metacg::pgis::PiraMCGProcessor::finalizeGraph(bool buildMarker) {
  if (graph.isEmpty()) {
    spdlog::get("errconsole")->error("Running the processor on empty graph. Need to construct graph.");
    exit(1);
  }

  if (graph.size() > 0) {
    // We assume that 'main' is always reachable.
    auto mainNode = graph.getMain();
    if (mainNode == nullptr) {
      spdlog::get("errconsole")->error("PiraMCGProcessor: Cannot find main function");
      exit(1);
    }
    mainNode->setReachable();

    // run reachability analysis -> mark reachable nodes
    CgHelper::reachableFromMainAnalysis(graph.getMain());

    // XXX This is a prerequisite for certain EstimatorPhases and should be demanded by them
    // CgHelper::calculateInclusiveStatementCounts(mainNode);
  }

  // also update all node attributes
  for (auto node : graph) {
    if (configPtr->samplesFile.empty()) {
      node->updateNodeAttributes();
    } else {
      node->updateNodeAttributes(false);
    }
  }
}

void metacg::pgis::PiraMCGProcessor::applyRegisteredPhases() {
  finalizeGraph();
  auto mainFunction = graph.getMain();

  if (mainFunction == nullptr) {
    spdlog::get("errconsole")->error("PiraMCGProcessor: Cannot find main function.");
    exit(1);
  }

  while (!phases.empty()) {
    EstimatorPhase *phase = phases.front();

    {  // RAII
      const std::string curPhase = phase->getName();
      metacg::RuntimeTimer rtt("Running " + curPhase);
      phase->doPrerequisites();
    }

    {  // RAII
      const std::string curPhase = phase->getName();
      metacg::RuntimeTimer rtt("Running " + curPhase);
      phase->modifyGraph(mainFunction);
      phase->generateReport();

      spdlog::get("console")->info("Print phase report");
      phase->printReport();

      CgReport report = phase->getReport();
      auto &gOpts = ::pgis::config::GlobalConfig::get();

      if (outputDotBetweenPhases) {
        printDOT(report.phaseName);
      }

      dumpInstrumentedNames(report);  // outputs the instrumentation

      if (gOpts.getAs<bool>(printUnwoundNames.cliName)) {
        dumpUnwoundNames(report);
      }

    }  // RAII

    phases.pop();
    // We don't know if estimator phases hold references / pointers to other EstimatorPhases
    // donePhases.push_back(std::shared_ptr<EstimatorPhase>{phase});
  }
}

int metacg::pgis::PiraMCGProcessor::getNumProcs() {
  int numProcs = 1;
  int prevNum = 0;
  for (auto node : graph) {
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

void metacg::pgis::PiraMCGProcessor::printDOT(std::string prefix) {
  std::string filename = "out/callgraph-" + configPtr->appName + "-" + prefix + ".dot";
  std::ofstream outfile(filename, std::ofstream::out);

  outfile << "digraph callgraph {\n";

  unsigned long long callsForThreePercentOfOverhead =
      configPtr->fastestPhaseOvSeconds * 10e9 * 0.03 / (double)CgConfig::nanosPerInstrumentedCall;

  int numProcs = getNumProcs();

  std::vector<std::string> whiteNodes;
  bool validList = false;
  bool wlEmpty = configPtr->whitelist.empty();
  if (!wlEmpty) {
    validList = readWhitelist(whiteNodes);
  }

  for (int i = 0; i < numProcs + 1; i++) {
    double procTime = 0;
    Callgraph procGraph;
    if (i < numProcs) {
      outfile << "subgraph cluster" << i << " {\n";
    }
    outfile << "node [shape=Mrecord]\n";
    for (auto node : graph) {
      if (wlEmpty || (validList && isNodeListed(whiteNodes, node->getFunctionName()))) {
        std::string functionName = node->getFunctionName();
        std::string attributes;
        std::string additionalLabel;
        std::string threadLabel;
        std::string additionalThreads;
        std::string nodeTime;
        //        double nodeTimeSum = 0;
        //        int activeThreads = 0;
        unsigned long long numCalls = 0;

        const auto &[hasBPD, bpd] = node->checkAndGet<BaseProfileData>();
        const auto &[hasPOD, pod] = node->checkAndGet<PiraOneData>();
        const auto &[hasPTD, ptd] = node->checkAndGet<PiraTwoData>();

        const auto getCalls = [&](const auto md) {
          if (hasBPD) {
            return md->getNumberOfCalls();
          } else {
            return 0ull;
          }
        };
        const auto getInclRT = [&](const auto md) {
          if (hasBPD) {
            return md->getInclusiveRuntimeInSeconds();
          } else {
            return .0;
          }
        };
        const auto getRT = [&](const auto md) {
          if (hasBPD) {
            return md->getRuntimeInSeconds();
          } else {
            return .0;
          }
        };
        const auto isFromCube = [&](const auto md) {
          if (hasPOD) {
            return md->comesFromCube();
          } else {
            return false;
          }
        };

        if (!threadLabel.empty() || (i == numProcs && node->get<BaseProfileData>()->getCgLocation().empty())) {
          procGraph.insert(node);
          if (node->hasUniqueCallPath()) {
            attributes += "color=blue, ";
          }
          if (CgHelper::isConjunction(node)) {
            attributes += "color=green, ";
          }
          if (node->isInstrumentedWitness()) {
            attributes += "style=filled, ";

            if (getCalls(bpd) > callsForThreePercentOfOverhead) {
              attributes += "fillcolor=red, ";
            } else {
              attributes += "fillcolor=grey, ";
            }
          }
          if (node->isInstrumentedConjunction()) {
            attributes += "style=filled, ";
            attributes += "fillcolor=palegreen, ";
          }

          additionalLabel += std::string("\\n #calls Total: ");
          additionalLabel += std::to_string(getCalls(bpd));
          additionalLabel += std::string("\\n #calls in Process: ");
          additionalLabel += std::to_string(numCalls);
          additionalLabel += std::string("\\n CubeInclRT: ");
          additionalLabel += std::to_string(getInclRT(bpd));
          additionalLabel += std::string("\\n IsFromCube: ");
          additionalLabel += std::string((isFromCube(pod) ? "True" : "False"));

          if (hasPTD) {
            auto conn = ptd->getExtrapModelConnector();
            if (conn.hasModels()) {
              if (!conn.isModelSet()) {
                auto &pConfig = ::pgis::config::ParameterConfig::get();
                ptd->getExtrapModelConnector().modelAggregation(pConfig.getPiraIIConfig()->modelAggregationStrategy);
              }
              auto &theModel = conn.getEPModelFunction();
              additionalLabel += '\n' + std::string(theModel->getAsString(this->epModelProvider.getParameterList()));
            }
          }

          // runtime & expectedSamples in node label
          std::string expectedSamples;
          outfile << "\"" << functionName << i << "\"[" << attributes << "label=\"{" << functionName << "\\n"
                  << "Total Time: " << getRT(bpd) << "s"
                  << "\\n"
                  << "|{ #samples: " << expectedSamples << additionalLabel << "}" << nodeTime << "|{"
                  << threadLabel.substr(0, threadLabel.size() - 1) << "}}\"]" << std::endl;
        }
      }
    }

    for (auto node : procGraph) {
      node->dumpToDot(outfile, i);
    }
    if (i < numProcs) {
      std::ostringstream procSum;
      procSum << procTime;
      outfile << "label = \"Process " << i << ": \\n" << procSum.str() << "s\"";
    }
    outfile << "\n}\n";
  }
  outfile << "\n}" << std::endl;
  outfile.close();
}

bool metacg::pgis::PiraMCGProcessor::readWhitelist(std::vector<std::string> &whiteNodes) {
  std::ifstream in(configPtr->whitelist.c_str());

  if (!in) {
    spdlog::get("errconsole")->error("Cannot open file {}", configPtr->whitelist);
    return false;
  }

  std::string str;
  while (std::getline(in, str)) {
    if (str.size() > 0) {
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

void metacg::pgis::PiraMCGProcessor::dumpInstrumentedNames(CgReport report) {
  if (noOutputRequired) {
    return;
  }

  std::string filename =
      configPtr->outputFile + "/instrumented-" + configPtr->appName + "-" + report.phaseName + ".txt";
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
    if (report.instrumentedNodes.empty()) {
      outfile << "aFunctionThatDoesNotExist" << std::endl;
    } else {
      for (auto name : report.instrumentedNames) {
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
    for (const auto &name : report.instrumentedNames) {
      ss << include << " " << name << "\n";
    }
    for (const auto &[name, node] : report.instrumentedPaths) {
      for (const auto &parent : node->getParentNodes()) {
        ss << include << " " << parent->getFunctionName() << " " << arrow << " " << name << "\n";
      }
    }

    // Edge instrumentation
    // XXX why?
    //    for (const auto &[parent, node] : report.instrumentedEdges) {
    //      ss << include << " " << parent->getFunctionName() << " " << arrow << " " << node->getFunctionName() << "\n";
    //    }
    ss << scorepEnd << "\n";

    outfile << ss.str();
  }
}

void metacg::pgis::PiraMCGProcessor::dumpUnwoundNames(CgReport report) {
  std::string filename = "out/unw-" + configPtr->appName + "-" + report.phaseName + ".txt";
  std::ofstream outfile(filename, std::ofstream::out);

  for (auto pair : report.unwoundNames) {
    std::string name = pair.first;
    int unwindSteps = pair.second;

    outfile << unwindSteps << " " << name << std::endl;
  }
}

Callgraph &metacg::pgis::PiraMCGProcessor::getCallgraph(PiraMCGProcessor *cg) { return cg->graph; }

void metacg::pgis::PiraMCGProcessor::attachExtrapModels() {
  epModelProvider.buildModels();
  for (const auto &n : graph) {
    spdlog::get("console")->debug("Attaching models for {}", n->getFunctionName());
    auto ptd = getOrCreateMD<PiraTwoData>(n, epModelProvider.getModelFor(n->getFunctionName()));
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

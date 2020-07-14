#include "CallgraphManager.h"

#include "ExtrapConnection.h"

#include "EXTRAP_Model.hpp"

#include "spdlog/spdlog.h"

#include <iomanip>  //  std::setw()

using namespace pira;

CallgraphManager::CallgraphManager(Config *config, extrapconnection::ExtrapConfig epCfg)
    : config(config), epModelProvider(epCfg) {}

CgNodePtr CallgraphManager::findOrCreateNode(std::string name, double timeInSeconds) {
  if (graph.hasNode(name)) {
    return graph.getLastSearched();
  } else {
    auto node = std::make_shared<CgNode>(name);
    auto bpd = new BaseProfileData();
    bpd->setRuntimeInSeconds(timeInSeconds);
    node->addMetaData(bpd);

    graph.insert(node);
    return node;
  }
}

void CallgraphManager::putNumberOfStatements(std::string name, int numberOfStatements, bool hasBody) {
  CgNodePtr node = findOrCreateNode(name);
  auto [has, obj] = node->checkAndGet<PiraOneData>();
  if (has) {
    obj->setNumberOfStatements(numberOfStatements);
    obj->setHasBody(hasBody);
  } else {
    auto *pod = new PiraOneData();
    pod->setNumberOfStatements(numberOfStatements);
    pod->setHasBody(hasBody);
    node->addMetaData(pod);
  }
}

void CallgraphManager::putNumberOfSamples(std::string name, unsigned long long numberOfSamples) {
  if (graph.hasNode(name)) {
    graph.getLastSearched()->setExpectedNumberOfSamples(numberOfSamples);
  }
}

void CallgraphManager::setNodeComesFromCube(std::string name) {
  auto node = findOrCreateNode(name);
  auto [has, obj] = node->checkAndGet<PiraOneData>();
  if (has) {
    obj->setComesFromCube();
  } else {
    assert(false && "Node has no PiraOneData");
  }
}

void CallgraphManager::putEdge(std::string parentName, std::string childName) {
  CgNodePtr parentNode = findOrCreateNode(parentName);
  CgNodePtr childNode = findOrCreateNode(childName);

  if (parentNode == nullptr || childNode == nullptr) {
    spdlog::get("errconsole")->warn("One of the edge nodes is a nullptr");
  }

  parentNode->addChildNode(childNode);
  childNode->addParentNode(parentNode);
}

void CallgraphManager::putEdge(std::string parentName, std::string parentFilename, int parentLine,
                               std::string childName, unsigned long long numberOfCalls, double timeInSeconds,
                               int threadId, int procId) {
  putEdge(parentName, childName);

  auto parentNode = graph.findNode(parentName);
  if (parentNode == nullptr) {
    spdlog::get("errconsole")->warn("Problem in looking up node");
  }
  parentNode->setFilename(parentFilename);
  parentNode->setLineNumber(parentLine);

  auto childNode = graph.findNode(childName);
  if (childNode) {
    auto bpd = new BaseProfileData();
    bpd->addCallData(parentNode, numberOfCalls, timeInSeconds, threadId, procId);
    childNode->addMetaData(bpd);
  } else {
    spdlog::get("errconsole")->warn("No Child {} found in graph", childName);
  }
}

void CallgraphManager::registerEstimatorPhase(EstimatorPhase *phase, bool noReport) {
  phases.push(phase);
  phase->injectConfig(config);
  phase->setGraph(&graph);

  if (noReport) {
    phase->setNoReport();
  }
}

void CallgraphManager::finalizeGraph(bool buildMarker) {
  if (graph.size() > 0) {
    // We assume that 'main' is always reachable.
    auto mainNode = graph.findMain();
    if (mainNode == nullptr) {
      spdlog::get("errconsole")->error("CallgraphManager: Cannot find main function");
      exit(1);
    }
    mainNode->setReachable();
  }

  // also update all node attributes
  for (auto node : graph) {
    if (config->samplesFile.empty()) {
      node->updateNodeAttributes();
    } else {
      node->updateNodeAttributes(false);
    }

    // graph.findMain caches the main node
    if (CgHelper::reachableFrom(graph.findMain(), node)) {
      spdlog::get("console")->trace("Setting reachable: {}", node->getFunctionName());
      node->setReachable();
    } else {
      node->setReachable(false);
    }

    if (buildMarker) {
      CgNodePtrSet markerPositions = CgHelper::getPotentialMarkerPositions(node);
      node->getMarkerPositions().insert(markerPositions.begin(), markerPositions.end());

      std::for_each(markerPositions.begin(), markerPositions.end(), [&node](const CgNodePtr &markerPosition) {
        markerPosition->getDependentConjunctions().insert(node);
      });
    }
  }
}

void CallgraphManager::applyRegisteredPhases() {
  finalizeGraph();
  auto mainFunction = graph.findMain();

  if (mainFunction == nullptr) {
    spdlog::get("errconsole")->error("CallgraphManager: Cannot find main function.");
    exit(1);
  }

  while (!phases.empty()) {
    EstimatorPhase *phase = phases.front();

#if BENCHMARK_PHASES
    auto startTime = std::chrono::system_clock::now();
#endif
    phase->modifyGraph(mainFunction);
    phase->generateReport();

    spdlog::get("console")->info("Print phase report");
    phase->printReport();

    CgReport report = phase->getReport();
#if PRINT_DOT_AFTER_EVERY_PHASE
    printDOT(report.phaseName);
#endif  // PRINT_DOT_AFTER_EVERY_PHASE
#if DUMP_INSTRUMENTED_NAMES
    dumpInstrumentedNames(report);
#endif  // DUMP_INSTRUMENTED_NAMES
#if DUMP_UNWOUND_NAMES
    dumpUnwoundNames(report);
#endif  // DUMP_UNWOUND_NAMES

#if BENCHMARK_PHASES
    auto endTime = std::chrono::system_clock::now();
    double calculationTime = (endTime - startTime).count() / 1e6;
    spdlog::get("console")->debug("Calculating phase {} took {} sec", phase->getName(), calculationTime);
#endif

    phases.pop();
    // We don't know if estimator phases hold references / pointers to other EstimatorPhases
    // donePhases.push_back(std::shared_ptr<EstimatorPhase>{phase});
  }

  if (!noOutputRequired) {
    // XXX Change to spdlog
    std::cout << " ---- "
              << "Fastest Phase: " << std::setw(8) << config->fastestPhaseOvPercent << " % with "
              << config->fastestPhaseName << std::endl;
  }

#if PRINT_FINAL_DOT
  printDOT("final");
#endif
}

int CallgraphManager::getNumProcs() {
  int numProcs = 1;
  int prevNum = 0;
  for (auto node : graph) {
    if (!node->getCgLocation().empty()) {
      for (CgLocation cgLoc : node->getCgLocation()) {
        if (cgLoc.get_procId() != prevNum) {
          prevNum = cgLoc.get_procId();
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

void CallgraphManager::printDOT(std::string prefix) {
  std::string filename = "out/callgraph-" + config->appName + "-" + prefix + ".dot";
  std::ofstream outfile(filename, std::ofstream::out);

  outfile << "digraph callgraph {\n";

  unsigned long long callsForThreePercentOfOverhead =
      config->fastestPhaseOvSeconds * 10e9 * 0.03 / (double)CgConfig::nanosPerInstrumentedCall;

  int numProcs = getNumProcs();

  std::vector<std::string> whiteNodes;
  bool validList = false;
  bool wlEmpty = config->whitelist.empty();
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
        double nodeTimeSum = 0;
        int activeThreads = 0;
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

        for (CgLocation cgLoc : node->getCgLocation()) {
          std::ostringstream threadTime;
          if (cgLoc.get_procId() == i) {
            threadTime << cgLoc.get_time();
            nodeTimeSum += cgLoc.get_time();
            numCalls += cgLoc.get_numCalls();

            if (config->showAllThreads) {
              threadLabel += "Thread " + std::to_string(cgLoc.get_threadId()) + ":\\n" + threadTime.str() + "s|";
            } else {
              if (activeThreads > 0) {
                additionalThreads +=
                    "Thread " + std::to_string(cgLoc.get_threadId()) + ":\\n" + threadTime.str() + "s|";
              } else {
                threadLabel += "Thread " + std::to_string(cgLoc.get_threadId()) + ":\\n" + threadTime.str() + "s|";
              }
              if (cgLoc.get_time() > 0) {
                activeThreads++;
              }
            }
          }
        }

        procTime += nodeTimeSum;

        if ((!config->showAllThreads && activeThreads > 1) || config->showAllThreads) {
          threadLabel += additionalThreads;
          std::ostringstream sumTime;
          sumTime << nodeTimeSum;
          nodeTime = "|{ Threads Total: " + sumTime.str() + "s}";
        }

        if (!threadLabel.empty() || (i == numProcs && node->getCgLocation().empty())) {
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
          if (node->isUnwound()) {
            // attributes += "shape=doubleoctagon, ";
            additionalLabel += std::string("\\n unwindSteps: ");
            additionalLabel += std::to_string(node->getNumberOfUnwindSteps());
          } else if (node->isLeafNode()) {
            // attributes += "shape=octagon, ";
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
                conn.useFirstModel();
              }
              auto theModel = conn.getEPModelFunction();
              additionalLabel += '\n' + std::string(theModel->getAsString(this->epModelProvider.getParameterList()));
            }
          }

          // runtime & expectedSamples in node label
          outfile << "\"" << functionName << i << "\"[" << attributes << "label=\"{" << functionName << "\\n"
                  << "Total Time: " << getRT(bpd) << "s"
                  << "\\n"
                  << "|{ #samples: " << node->getExpectedNumberOfSamples() << additionalLabel << "}" << nodeTime << "|{"
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

  // std::cout << "DOT file dumped (" << filename << ")." << std::endl;
}

bool CallgraphManager::readWhitelist(std::vector<std::string> &whiteNodes) {
  std::ifstream in(config->whitelist.c_str());

  if (!in) {
    spdlog::get("errconsole")->error("Cannot open file {}", config->whitelist);
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

bool CallgraphManager::isNodeListed(std::vector<std::string> whiteNodes, std::string node) {
  for (auto wNode : whiteNodes) {
    if (node == wNode) {
      return true;
    }
  }
  return false;
}

void CallgraphManager::dumpInstrumentedNames(CgReport report) {
  if (noOutputRequired) {
    return;
  }

  std::string filename = config->outputFile + "/instrumented-" + config->appName + "-" + report.phaseName + ".txt";
  std::size_t found = filename.find(config->outputFile + "/instrumented-" + config->appName + "-" + "Incl");
  if (found != std::string::npos) {
    filename = config->outputFile + "/instrumented-" + config->appName + ".txt";
  }
  filename = config->outputFile + "/instrumented-" + config->appName + ".txt";
  spdlog::get("console")->info("Writing to {}", filename);
  std::ofstream outfile(filename, std::ofstream::out);

  // The simple whitelist used so far in PIRA
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
    for (const auto name : report.instrumentedNames) {
      ss << include << " " << name << "\n";
    }
    for (const auto [name, node] : report.instrumentedPaths) {
      for (const auto parent : node->getParentNodes()) {
        ss << include << " " << parent->getFunctionName() << " " << arrow << " " << name << "\n";
      }
    }
    ss << scorepEnd << "\n";

    outfile << ss.str();
  }
}

void CallgraphManager::dumpUnwoundNames(CgReport report) {
  std::string filename = "out/unw-" + config->appName + "-" + report.phaseName + ".txt";
  std::ofstream outfile(filename, std::ofstream::out);

  for (auto pair : report.unwoundNames) {
    std::string name = pair.first;
    int unwindSteps = pair.second;

    outfile << unwindSteps << " " << name << std::endl;
  }
}

Callgraph &CallgraphManager::getCallgraph(CallgraphManager *cg) { return cg->graph; }

void CallgraphManager::attachExtrapModels() {
  epModelProvider.buildModels();
  for (const auto n : graph) {
    spdlog::get("console")->debug("Attaching models for {}", n->getFunctionName());
    auto ptd = getOrCreateMD<PiraTwoData>(n, epModelProvider.getModelFor(n->getFunctionName()));
    if (!ptd->getExtrapModelConnector().hasModels()) {
      spdlog::get("console")->trace("attachExtrapModels hasModels == false -> Setting new ModelConnector");
      ptd->setExtrapModelConnector(epModelProvider.getModelFor(n->getFunctionName()));
    }

    ptd->getExtrapModelConnector().setEpolator(extrapconnection::ExtrapExtrapolator(epModelProvider.getConfigValues()));

    if (ptd->getExtrapModelConnector().hasModels()) {
      spdlog::get("console")->trace("attachExtrapModels hasModels == true -> Use first attached model.");
      ptd->getExtrapModelConnector().useFirstModel();
    }
    spdlog::get("console")->debug("Model is set {}", n->get<PiraTwoData>()->getExtrapModelConnector().isModelSet());
  }
  spdlog::get("console")->info("Attaching Extra-P models done");
}

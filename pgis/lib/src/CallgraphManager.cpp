#include "CallgraphManager.h"

#include "ExtrapConnection.h"

#include "EXTRAP_Model.hpp"

CallgraphManager::CallgraphManager(Config *config, extrapconnection::ExtrapConfig epCfg)
    : config(config), epModelProvider(epCfg) {}

CgNodePtr CallgraphManager::findOrCreateNode(std::string name, double timeInSeconds) {
  if (graph.hasNode(name)) {
    return graph.getLastSearched();
  } else {
    auto node = std::make_shared<CgNode>(name);
    graph.insert(node);
    node->setRuntimeInSeconds(timeInSeconds);
    return node;
  }
}

void CallgraphManager::putNumberOfStatements(std::string name, int numberOfStatements) {
  CgNodePtr node = findOrCreateNode(name);
  node->setNumberOfStatements(numberOfStatements);
}

void CallgraphManager::putNumberOfSamples(std::string name, unsigned long long numberOfSamples) {
  if (graph.hasNode(name)) {
    graph.getLastSearched()->setExpectedNumberOfSamples(numberOfSamples);
  }
}

void CallgraphManager::setNodeComesFromCube(std::string name) {
  auto node = findOrCreateNode(name);
  node->setComesFromCube();
}

void CallgraphManager::putEdge(std::string parentName, std::string childName) {
  CgNodePtr parentNode = findOrCreateNode(parentName);
  CgNodePtr childNode = findOrCreateNode(childName);

  if (parentNode == nullptr || childNode == nullptr) {
    std::cerr << "[warning] One of the edge nodes is a nullptr\n";
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
    std::cerr << "ERROR in looking up node." << std::endl;
  }
  parentNode->setFilename(parentFilename);
  parentNode->setLineNumber(parentLine);

  auto childNode = graph.findNode(childName);
  if (childNode) {
    childNode->addCallData(parentNode, numberOfCalls, timeInSeconds, threadId, procId);
  } else {
    std::cerr << "No child " << childName << " found in graph\n";
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
      std::cerr << "CallgraphManager: Cannot find main function." << std::endl;
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
      node->setReachable();
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
    std::cerr << "CallgraphManager: Cannot find main function." << std::endl;
    exit(1);
  }

  while (!phases.empty()) {
    EstimatorPhase *phase = phases.front();

#if BENCHMARK_PHASES
    auto startTime = std::chrono::system_clock::now();
#endif
    phase->modifyGraph(mainFunction);
    phase->generateReport();
    
    std::cout << "## Print Phase Report" << std::endl;
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
    std::cout << "\t- "
              << "calculation took " << calculationTime << " sec" << std::endl;
#endif

    phases.pop();
    // We don't know if estimator phases hold references / pointers to other EstimatorPhases
    // donePhases.push_back(std::shared_ptr<EstimatorPhase>{phase});
  }

  if (!noOutputRequired) {
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

            if (node->getNumberOfCalls() > callsForThreePercentOfOverhead) {
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
          additionalLabel += std::to_string(node->getNumberOfCalls());
          additionalLabel += std::string("\\n #calls in Process: ");
          additionalLabel += std::to_string(numCalls);
          additionalLabel += std::string("\\n CubeInclRT: ");
          additionalLabel += std::to_string(node->getInclusiveRuntimeInSeconds());
          additionalLabel += std::string("\\n IsFromCube: ");
          additionalLabel += std::string((node->comesFromCube() ? "True" : "False"));

          if (node->getExtrapModelConnector().hasModels()) {
            if (!node->getExtrapModelConnector().isModelSet()) {
              node->getExtrapModelConnector().useFirstModel();
            }
            auto theModel = node->getExtrapModelConnector().getEPModelFunction();
            additionalLabel += '\n' + std::string(theModel->getAsString(this->epModelProvider.getParameterList()));
          }

          // runtime & expectedSamples in node label
          outfile << "\"" << functionName << i << "\"[" << attributes << "label=\"{" << functionName << "\\n"
                  << "Total Time: " << node->getRuntimeInSeconds() << "s"
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
    std::cerr << "Cannot open the File : " << config->whitelist << std::endl;
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
  std::cout << "Writing to " << filename << std::endl;
  std::ofstream outfile(filename, std::ofstream::out);

  if (report.instrumentedNodes.empty()) {
    outfile << "aFunctionThatDoesNotExist" << std::endl;
  } else {
    for (auto name : report.instrumentedNames) {
      outfile << name << std::endl;
    }
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
  for (const auto &n : graph) {
    // std::cout << n->getFunctionName() << "\n";
    n->setExtrapModelConnector(epModelProvider.getModelFor(n->getFunctionName()));
    n->getExtrapModelConnector().setEpolator(extrapconnection::ExtrapExtrapolator(epModelProvider.getConfigValues()));
  }
  std::cout << "Attaching Extra-P Models done." << std::endl;
}

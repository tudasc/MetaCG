#include "ProximityMeasureEstimatorPhase.h"

ProximityMeasureEstimatorPhase::ProximityMeasureEstimatorPhase(std::string filename)
    : EstimatorPhase("proximity-estimator"),
      filename(filename),
      compareAgainst(CubeCallgraphBuilder::build(filename, config)) {
  std::cout << "Everything constructed" << std::endl;
}

void ProximityMeasureEstimatorPhase::modifyGraph(CgNodePtr mainMethod) {
  if (mainMethod == nullptr) {
    std::cerr << "Main method is NULL" << std::endl;
    return;
  }

  // Prepare the Callgraph for the processing
  std::for_each(graph->begin(), graph->end(), [this](const CgNodePtr n) { this->getInclusiveAndChildrenRuntime(n); });

  std::for_each(compareAgainst.begin(), compareAgainst.end(),
                [this](const CgNodePtr n) { this->getInclusiveAndChildrenRuntime(n); });

  workQ.clear();

  // Fill the set of nodes to iterate over for comparison
  bool collectInfoForComparingProfileOnly(false);
  if (collectInfoForComparingProfileOnly) {
    for (auto iter = compareAgainst.begin(); iter != compareAgainst.end(); ++iter) {
      if (workQ.find(*iter) == workQ.end())
        workQ.insert(*iter);
    }
  } else {
    prepareList(workQ, mainMethod);
  }

  // compute dominance and severity
  for (const auto &n : workQ) {
    std::map<CgNodePtr, double> innerDomMap = buildDominanceMap(n);
    std::map<CgNodePtr, double> innerSevMap = buildSeverityMap(n);
    domMap.insert(innerDomMap.begin(), innerDomMap.end());
    sevMap.insert(innerSevMap.begin(), innerSevMap.end());
  }
}

void ProximityMeasureEstimatorPhase::printReport() {
  std::cout << "==== ProximityMeasure Reporter ====\n";
  std::cerr << "Running estimation" << std::endl;
  long numFuncsFull = graph->size();
  long numFuncsOther = compareAgainst.size();
  std::cout << numFuncsFull / numFuncsOther << "\% of the originally recorded functions preserved" << std::endl;

  // calculate penalty for not existing nodes
  double penalty = 0.0;

  // According to our definition nodes do only add penalty if paths to those
  // nodes are no longer reconstructible
  std::set<CgNodePtr> penaltyNodes;

  for_each(graph->begin(), graph->end(), [this, &penalty, &penaltyNodes](const CgNodePtr &n) {
    Callgraph::ContainerT::iterator start = std::find_if(compareAgainst.begin(), compareAgainst.end(),
                                                         [n](const CgNodePtr &ptr) { return ptr->isSameFunction(n); });

    if (start == compareAgainst.end()) {
      penaltyNodes.insert(n);
      // This is the case where we lost a node!
      for (const auto &c : n->getChildNodes())
        penalty += n->getDominance(c);
    }
  });

  //	for(const auto &pn : penaltyNodes){
  //		std::cout << pn->getFunctionName().substr(0,50) << "...\n";
  //	}
  //	std::cout << std::endl;

  std::cout << "Overall penalty: " << penalty << std::endl;

  /**
   * We try to find a good severity normalization here. That is a function that
   * indicates whether it makes sense to capture the function using
   * instrumentation. Especially this should give an indication whether or not
   * the function is too small to instrument or too large to use plain
   * instrumentation.
   * TODO: develop a good function/metric/heuristic which determines which
   * functions are interesting in that sense.
   * TODO: implement something that tells you about the probe distribution.
   */
  typedef std::pair<CgNodePtr, double> sevListValT;
  std::vector<sevListValT> severityList(sevMap.size());
  std::copy(sevMap.begin(), sevMap.end(), severityList.begin());
  auto iter = std::max_element(severityList.begin(), severityList.end(),
                               [](const sevListValT &a, const sevListValT &b) { return a.second < b.second; });
  if (iter == severityList.end()) {
    std::cerr << "NULLPTR encountered not providing estimation" << std::endl;
    return;
  }
  auto t = *iter;
  if (t.first == nullptr) {
    std::cerr << "NULLPTR encountered not providing estimation" << std::endl;
    return;
  }
  double maxVal = t.second;  // This gave weird results in the first place
  //  maxVal = 1000; // FIXME what would be the value we normlize our data to?

  std::for_each(severityList.begin(), severityList.end(), [maxVal](sevListValT &elem) {
    double curNormalized = elem.second / maxVal;
    elem.second = std::log(10 * curNormalized) + std::log(10 * curNormalized) - curNormalized;
  });

  std::sort(severityList.begin(), severityList.end(),
            [](const sevListValT &a, const sevListValT &b) { return a.second > b.second; });

  std::cout << "Severity List (sorted): \n";
  int numEntriesShown = 0;
  for (const auto &p : severityList) {
    if (p.second > -1) {
      std::cout << p.first->getFunctionName().substr(0, 50) << "...: " << p.second;
      if (getCorrespondingComparisonNode(p.first) != nullptr) {
        std::cout << "  \t[Preserved: yes]\n";
      } else {
        std::cout << "  \t[Preserved: no]\n";
      }
      numEntriesShown++;
    }
  }
  std::cout << "Showing " << numEntriesShown << " with value > -1 from total of " << severityList.size() << std::endl;
}

double ProximityMeasureEstimatorPhase::childrenPreservedMetric(CgNodePtr origFunc) {
  std::set<CgNodePtr> worklist;
  prepareList(worklist, origFunc);

  double val = 0.0;
  // for all functions in original CG, find corresponding node in filtered CG
  // if node is found calculate childrenPreserved(orig, filtered)
  for (auto &origNode : worklist) {
    CgNodePtr funcNode = getCorrespondingComparisonNode(origNode);

    if (funcNode == nullptr)
      continue;

    double cs = childrenPreserved(origNode, funcNode);
    val += cs;
  }
  return val / worklist.size();
}

// returns the CgNode with the same function as node
CgNodePtr ProximityMeasureEstimatorPhase::getCorrespondingComparisonNode(const CgNodePtr node) {
  for (auto fNode = compareAgainst.begin(); fNode != compareAgainst.end(); ++fNode) {
    CgNodePtr funcNode = *fNode;

    if (node->isSameFunction(funcNode)) {
      return funcNode;
    }
  }
  return nullptr;
}

double ProximityMeasureEstimatorPhase::portionOfRuntime(CgNodePtr node) {
  std::pair<double, double> runtime = getInclusiveAndChildrenRuntime(node);
  // std::pair<double, double> runtime(1.0, 1.0);

  CgNodePtr compNode = getCorrespondingComparisonNode(node);
  if (compNode == nullptr) {
    return 0.0;
  }

  std::pair<double, double> runtimeInCompareProfile = getInclusiveAndChildrenRuntime(compNode);

  std::cout.precision(12);
  std::cout << "====\nRuntime: " << runtime.first << "\nCompared runtime: " << runtimeInCompareProfile.first
            << "\nRuntime of children: " << runtime.second
            << "\nRuntime of children: " << runtimeInCompareProfile.second << "\n====" << std::endl;
  std::cout << "==== Ratio between runtimes ====\nOriginal profile: " << runtime.second / runtime.first
            << "\nCompared profile: " << runtimeInCompareProfile.second / runtimeInCompareProfile.first << "\n"
            << std::endl;

  return runtime.second / runtime.first;
}

std::map<CgNodePtr, double> ProximityMeasureEstimatorPhase::buildDominanceMap(CgNodePtr node) {
  // we build a map from CgNodePtr to double vals
  std::map<CgNodePtr, double> dominance;

  std::set<CgNodePtr> worklist;

  prepareListOneLevel(worklist, node);
  double rt = node->getInclusiveRuntimeInSeconds();
  for (const auto &c : worklist) {
    if (c == node) {
      continue;
    }
    unsigned long long numCalls = c->getNumberOfCalls(node);
    unsigned long long totalNumberOfCalls = c->getNumberOfCallsWithCurrentEdges();
    if (numCalls == 0 || totalNumberOfCalls == 0) {
      node->setDominance(c, .0);
      continue;
    }
    if ((numCalls / totalNumberOfCalls) == 0) {
      node->setDominance(c, .0);
      continue;
    }

    // add a little eps to the runtime in case it is 0.0
    double rtis = c->getInclusiveRuntimeInSeconds() + 1e-16;
    double partRTIS = (double(numCalls / totalNumberOfCalls) * (rtis));
    double v = rt * partRTIS;

    if (v == std::numeric_limits<double>::infinity()) {
      node->setDominance(c, .0);
    } else {
      node->setDominance(c, v);
    }
  }
  return dominance;
}

std::map<CgNodePtr, double> ProximityMeasureEstimatorPhase::buildSeverityMap(CgNodePtr node) {
  std::map<CgNodePtr, double> severityMap;

  auto rt = node->getRuntimeInSeconds();
  unsigned long long totalNumberOfCalls = node->getNumberOfCallsWithCurrentEdges();
  if (totalNumberOfCalls == 0) {
    severityMap[node] = .0;
    return severityMap;
  }
  double rtPerCall = rt / totalNumberOfCalls;
  // std::cout << "Runtime per call for " <<
  // node->getFunctionName().substr(0,50) << "...: " << rtPerCall << std::endl;
  double probeCost = 1e-6;  // FIXME have a real value here!
  double v = rtPerCall / probeCost;

  severityMap[node] = v;
  return severityMap;
}

std::pair<double, double> ProximityMeasureEstimatorPhase::getInclusiveAndChildrenRuntime(CgNodePtr node) {
  std::set<CgNodePtr> worklist;
  prepareList(worklist, node);

  double runtimeInSeconds = node->getRuntimeInSeconds();
  // the init value is -runtimeInSeconds, because worklist contains "node"
  // which we don't want to include in the summation
  double runtimeSumOfChildrenInSeconds =
      std::accumulate(worklist.begin(), worklist.end(), -runtimeInSeconds,
                      [](const double &a, const CgNodePtr &b) { return a + b->getRuntimeInSeconds(); });

  runtimeInSeconds += runtimeSumOfChildrenInSeconds;
  node->setInclusiveRuntimeInSeconds(runtimeInSeconds);
  return std::make_pair(runtimeInSeconds, runtimeSumOfChildrenInSeconds);
}

double ProximityMeasureEstimatorPhase::childrenPreserved(CgNodePtr orig, CgNodePtr filtered) {
  if (orig->getChildNodes().size() == 0) {
    if (filtered->getChildNodes().size() > 0) {
      std::cerr << "[Warning]: Should not happen!\n\t>>No child nodes in "
                   "original profile but filtered profile."
                << std::endl;
    }
    return 1.0;
  }
  return double(filtered->getChildNodes().size()) / orig->getChildNodes().size();
}

void ProximityMeasureEstimatorPhase::prepareList(std::set<CgNodePtr> &worklist, CgNodePtr mainM) {
  if (mainM == nullptr) {
    std::cerr << "NULLPTR DETECTED" << std::endl;
  }
  worklist.insert(mainM);
  for (const auto n : mainM->getChildNodes()) {
    if (worklist.find(n) == worklist.end()) {
      //		std::cout << "\n" << n->getFunctionName() << " in " <<
      // mainM->getFunctionName() << " " << mainM->getChildNodes().size() <<
      // std::endl;
      prepareList(worklist, n);
    }
  }
}

void ProximityMeasureEstimatorPhase::prepareListOneLevel(std::set<CgNodePtr> &worklist, CgNodePtr root) {
  worklist.insert(root);
  for (const auto &n : root->getChildNodes()) {
    worklist.insert(n);
  }
}

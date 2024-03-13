//
// Created by jp on 12.07.22.
//

#include "EstimatorPhase.h"
#include "MetaData/PGISMetaData.h"
#include <fstream>
#include <iostream>

//// WL INSTR ESTIMATOR PHASE

WLInstrEstimatorPhase::WLInstrEstimatorPhase(const std::string &wlFilePath) : EstimatorPhase("WLInstr", nullptr) {
  std::ifstream ifStream(wlFilePath);
  if (!ifStream.good()) {
    metacg::MCGLogger::instance().getErrConsole()->error("Unable to open Wl file {}", wlFilePath);
  }

  std::string buff;
  while (getline(ifStream, buff)) {
    whiteList.insert(std::hash<std::string>()(buff));
  }
}

void WLInstrEstimatorPhase::modifyGraph(metacg::CgNode *mainMethod) {
  for (const auto &elem : graph->getNodes()) {
    const auto &node = elem.second.get();
    if (whiteList.find(node->getId()) != whiteList.end()) {
      metacg::pgis::instrumentNode(node);
    }
  }
}

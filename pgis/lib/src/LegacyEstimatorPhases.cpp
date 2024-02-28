//
// Created by jp on 12.07.22.
//

#include "ErrorCodes.h"
#include "EstimatorPhase.h"
#include "MetaData/PGISMetaData.h"
#include <filesystem>
#include <fstream>
#include <iostream>

//// WL INSTR ESTIMATOR PHASE

WLInstrEstimatorPhase::WLInstrEstimatorPhase(const std::filesystem::path &wlFilePath)
    : EstimatorPhase("WLInstr", nullptr), wlFilePath(wlFilePath) {}

void WLInstrEstimatorPhase::init() {
  if (isInitialized) {
    return;
  }

  std::ifstream ifStream(wlFilePath);
  if (!ifStream.good()) {
    metacg::MCGLogger::instance().getErrConsole()->error("Unable to open Wl file {}", wlFilePath.string());
    exit(metacg::pgis::ErrorCode::FileDoesNotExist);
  }

  std::string buff;
  while (getline(ifStream, buff)) {
    whiteList.insert(std::hash<std::string>()(buff));
  }
  isInitialized = true;
}

void WLInstrEstimatorPhase::modifyGraph(metacg::CgNode *mainMethod) {
  for (const auto &elem : graph->getNodes()) {
    const auto &node = elem.second.get();
    if (whiteList.find(node->getId()) != whiteList.end()) {
      metacg::pgis::instrumentNode(node);
    }
  }
}

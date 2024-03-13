/**
 * File: OnlyMainEstimatorPhase.cpp
 * License: Part of the metacg project. Licensed under BSD 3 clause license. See LICENSE.txt file at
 * https://github.com/tudasc/metacg/LICENSE.txt
 */

#include "loadImbalance/OnlyMainEstimatorPhase.h"
#include "MetaData/PGISMetaData.h"

using namespace metacg;
using namespace LoadImbalance;

OnlyMainEstimatorPhase::OnlyMainEstimatorPhase(Callgraph *callgraph)
    : EstimatorPhase("OnlyMainEstimatorPhase", callgraph) {}

void OnlyMainEstimatorPhase::modifyGraph(metacg::CgNode *mainMethod) {
  //  mainMethod->setState(CgNodeState::INSTRUMENT_WITNESS);
  metacg::pgis::instrumentNode(mainMethod);
}

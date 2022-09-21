/**
 * File: OnlyMainEstimatorPhase.cpp
 * License: Part of the metacg project. Licensed under BSD 3 clause license. See LICENSE.txt file at
 * https://github.com/tudasc/metacg/LICENSE.txt
 */

#include "loadImbalance/OnlyMainEstimatorPhase.h"
#include "MetaData/PGISMetaData.h"

using namespace metacg;
using namespace LoadImbalance;

OnlyMainEstimatorPhase::OnlyMainEstimatorPhase() : EstimatorPhase("OnlyMainEstimatorPhase") {}

OnlyMainEstimatorPhase::~OnlyMainEstimatorPhase() noexcept {}

void OnlyMainEstimatorPhase::modifyGraph(CgNodePtr mainMethod) {
//  mainMethod->setState(CgNodeState::INSTRUMENT_WITNESS);
  pgis::instrumentNode(mainMethod);
}

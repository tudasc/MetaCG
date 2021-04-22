/**
 * File: OnlyMainEstimatorPhase.cpp
 * License: Part of the MetaCG project. Licensed under BSD 3 clause license. See LICENSE.txt file at https://github.com/tudasc/metacg/LICENSE.txt
 */

#include "loadImbalance/OnlyMainEstimatorPhase.h"

using namespace LoadImbalance;

OnlyMainEstimatorPhase::OnlyMainEstimatorPhase() : EstimatorPhase("OnlyMainEstimatorPhase") {}

OnlyMainEstimatorPhase::~OnlyMainEstimatorPhase() noexcept {}

void OnlyMainEstimatorPhase::modifyGraph(CgNodePtr mainMethod) {
  mainMethod->setState(CgNodeState::INSTRUMENT_WITNESS);
}

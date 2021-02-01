#include "loadImbalance/OnlyMainEstimatorPhase.h"

using namespace LoadImbalance;

OnlyMainEstimatorPhase::OnlyMainEstimatorPhase() : EstimatorPhase("OnlyMainEstimatorPhase") {}

OnlyMainEstimatorPhase::~OnlyMainEstimatorPhase() noexcept {}

void OnlyMainEstimatorPhase::modifyGraph(CgNodePtr mainMethod) {
  mainMethod->setState(CgNodeState::INSTRUMENT_WITNESS);
}

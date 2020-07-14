#include "SanityCheckEstimatorPhase.h"

SanityCheckEstimatorPhase::SanityCheckEstimatorPhase() : EstimatorPhase("SanityCheck", true), numberOfErrors(0) {}

SanityCheckEstimatorPhase::~SanityCheckEstimatorPhase() {}

void SanityCheckEstimatorPhase::modifyGraph(CgNodePtr mainMethod) {
  for (auto node : (*graph)) {
    // unwound nodes are fine as they are
    if (!CgHelper::isConjunction(node) || node->isInstrumentedConjunction()) {
      continue;
    }

    if (node->isUnwound()) {
      if (CgHelper::isOnCycle(node)) {
        std::cerr << "ERROR: unwound function is on a circle: " << node->getFunctionName() << std::endl;
        numberOfErrors++;
      } else {
        continue;
      }

      if (node->getNumberOfUnwindSteps() == 0) {
        std::cerr << "ERROR: unwound function with 0 unwindSteps" << std::endl;
        numberOfErrors++;
      }
    }

    numberOfErrors += (CgHelper::isUniquelyInstrumented(node, nullptr, true) ? 0 : 1);
    numberOfErrors += CgHelper::uniquelyInstrumentedConjunctionTest(node, true);
  }

  // XXX idea: check that there is no instrumentation below unwound nodes
}

void SanityCheckEstimatorPhase::printAdditionalReport() {
  std::cout << "\t"
            << "SanityCheck done with " << numberOfErrors << " errors." << std::endl;
}

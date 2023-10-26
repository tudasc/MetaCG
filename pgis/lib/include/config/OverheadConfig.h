#ifndef METACG_OVERHEADCONFIG_H
#define METACG_OVERHEADCONFIG_H

#include "nlohmann/json.hpp"

namespace pgis::config {
struct OverheadConfig {
  double thresholdHotspotInclusive =
      0.2;  // Functions that have an inclusive runtime >= thresholdHotspotInclusive * main inclusive runtime are
            // considered hotspots and only removed if no non hotspot node can be removed  JR Thesis 4.1.2
  double thresholdHotspotExclusive =
      0.1;  // Functions that have an exclsuive (can be part inclusive if not all children are instrumented) runtime >=
            // thresholdHotspotExclusive * main inclusive runtime are considered hotspots and only removed if no non
            // hotspot node can be removed  JR Thesis 4.1.2
  double percentageExplorationBudget =
      0.3;  // How much of the exploration budget (calls that can be instrumented) should at least be used to explore
            // new nodes, even if this requires the kicking of previous instrumented nodes. JR Thesis 4.1.3
  int inlineScaleFactor = 100;     // Factor to multiply the cost of instrumenting inlined functions with JR Thesis 4.4
  int recursionScaleFactor = 100;  // Factor to multiply the cost of instrumenting self recursive functions or pairs of
                                   // two functions that call each other with JR Thesis 4.4
  // MPI functions don't have an accessible body and no statement count either. This is used as an estimate for it
  int mpiFunctionsStatementEstimate = 100;
};

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(OverheadConfig, thresholdHotspotInclusive, thresholdHotspotExclusive,
                                   percentageExplorationBudget, inlineScaleFactor, recursionScaleFactor,
                                   mpiFunctionsStatementEstimate)

}  // namespace pgis::config

#endif  // METACG_OVERHEADCONFIG_H

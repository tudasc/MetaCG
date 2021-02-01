
#ifndef CGNODEHELPER_H_
#define CGNODEHELPER_H_

#include "CgNode.h"

#include <algorithm>  // std::set_intersection
#include <memory>
#include <queue>

#include <cassert>

class Callgraph;

// XXX These numbers probably go away.
namespace CgConfig {
const unsigned long long nanosPerInstrumentedCall = 7;

const unsigned long long nanosPerUnwindSample = 0;
const unsigned long long nanosPerUnwindStep = 1000;

const unsigned long long nanosPerNormalProbe = 220;
const unsigned long long nanosPerMPIProbe = 200;

const unsigned long long nanosPerSample = 4500;  // PAPI timers
//	const unsigned long long nanosPerSample
//= 2000;	// itimers

const unsigned long long nanosPerHalfProbe = 105;

extern int samplesPerSecond;
}  // namespace CgConfig

struct Config {
  double referenceRuntime = .0;
  double actualRuntime = .0;
  double totalRuntime = .0;
  std::string otherPath = "";
  bool useMangledNames = true;
  int nanosPerHalfProbe = CgConfig::nanosPerHalfProbe;
  std::string appName = "";
  bool tinyReport = false;
  bool ignoreSamplingOv = false;

  std::string fastestPhaseName = "NO_PHASE";
  double fastestPhaseOvPercent = 1e9;
  double fastestPhaseOvSeconds = 1e9;

  bool greedyUnwind = false;
  std::string samplesFile = "";
  std::string outputFile{"out"};
  bool showAllThreads = false;
  std::string whitelist = "";
};

namespace CgHelper {

bool isConjunction(CgNodePtr node);

int uniquelyInstrumentedConjunctionTest(CgNodePtr conjunctionNode, bool printErrors = true);
bool isUniquelyInstrumented(CgNodePtr conjunctionNode, CgNodePtr unInstrumented = nullptr, bool printErrors = true);
CgNodePtrSet getInstrumentationPath(CgNodePtr start);

bool deleteInstrumentationIfRedundant(CgNodePtr node);
bool allParentsPathsInstrumented(CgNodePtr conjunctionNode);

unsigned long long getInstrumentationOverheadOfConjunction(CgNodePtr conjunctionNode);
unsigned long long getInstrumentationOverheadOfConjunction(CgNodePtrSet conjunctionNodes);
unsigned long long getInstrumentationOverheadServingOnlyThisConjunction(CgNodePtr conjunctionNode);
unsigned long long getInstrumentationOverheadServingOnlyThisConjunction(CgNodePtrSet conjunctionNodes);
unsigned long long getInstrumentationOverheadOfPath(CgNodePtr node);
CgNodePtr getInstrumentedNodeOnPath(CgNodePtr node);

// Graph Stats
CgNodePtrSet getPotentialMarkerPositions(CgNodePtr conjunction);
bool isValidMarkerPosition(CgNodePtr markerPosition, CgNodePtr conjunction);
bool isOnCycle(CgNodePtr node);
CgNodePtrSet getReachableConjunctions(CgNodePtrSet markerPositions);

/**
 * Runs analysis on the call-graph and marks every node which is reachable from main
 */
void reachableFromMainAnalysis(CgNodePtr mainNode);

/**
 * Calculates the inclusive statement count for every node which is reachable from mainNode and saves in the node field
 */
void calculateInclusiveStatementCounts(CgNodePtr mainNode);

bool reachableFrom(CgNodePtr parentNode, CgNodePtr childNode);
CgNodePtrUnorderedSet allNodesToMain(CgNodePtr startNode, CgNodePtr mainNode,
                                     const std::unordered_map<CgNodePtr, CgNodePtrUnorderedSet> &init);
CgNodePtrUnorderedSet allNodesToMain(CgNodePtr startNode, CgNodePtr mainNode);

bool removeInstrumentationOnPath(CgNodePtr node);

bool isConnectedOnSpantree(CgNodePtr n1, CgNodePtr n2);
bool canReachSameConjunction(CgNodePtr n1, CgNodePtr n2);

CgNodePtrSet getDescendants(CgNodePtr child);
CgNodePtrSet getAncestors(CgNodePtr child);

void markReachablePar(CgNodePtr start, CgNodePtrUnorderedSet &seen);

void markReachableParStart(CgNodePtr start);

double calcRuntimeThreshold(const Callgraph &cg, bool useLongAsRef);

double calcInclusiveRuntime(CgNode *node);

inline CgNodePtrSet setIntersect(const CgNodePtrSet &a, const CgNodePtrSet &b) {
  CgNodePtrSet intersect;

  std::set_intersection(a.begin(), a.end(), b.begin(), b.end(), std::inserter(intersect, intersect.begin()));

  return intersect;
}

inline CgNodePtrSet setDifference(const CgNodePtrSet &a, const CgNodePtrSet &b) {
  CgNodePtrSet difference;

  std::set_difference(a.begin(), a.end(), b.begin(), b.end(), std::inserter(difference, difference.begin()));

  return difference;
}

inline bool isSubsetOf(const CgNodePtrSet &smallSet, const CgNodePtrSet &largeSet) {
  return setDifference(smallSet, largeSet) == smallSet;
}

inline bool intersects(const CgNodePtrSet &a, const CgNodePtrSet &b) { return !setIntersect(a, b).empty(); }
}  // namespace CgHelper

#endif

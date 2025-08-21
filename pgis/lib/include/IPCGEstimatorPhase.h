/**
 * File: IPCGEstimatorPhase.h
 * License: Part of the metacg project. Licensed under BSD 3 clause license. See LICENSE.txt file at
 * https://github.com/tudasc/metacg/LICENSE.txt
 */

#ifndef IPCGESTIMATORPHASE_H_
#define IPCGESTIMATORPHASE_H_

#include "EstimatorPhase.h"
#include "MetaData/CgNodeMetaData.h"

#include <map>
#include <optional>
#include <queue>
#include <set>
#include <utility>

class AttachInstrumentationResultsEstimatorPhase : public EstimatorPhase {
 public:
  AttachInstrumentationResultsEstimatorPhase(metacg::Callgraph* callgraph);
  void modifyGraph(metacg::CgNode* mainMethod) override;

 protected:
  void printReport() override;
};

/** RN: instrument the first n levels starting from main */
class FirstNLevelsEstimatorPhase : public EstimatorPhase {
 public:
  FirstNLevelsEstimatorPhase(int levels);
  ~FirstNLevelsEstimatorPhase();

  void modifyGraph(metacg::CgNode* mainMethod);

 private:
  void instrumentLevel(metacg::CgNode* parentNode, int levelsLeft);

  const int levels;
};

/**
 * Class to fill gaps in the list of functions to instrument. For each function selected for instrumentation, all paths
 * from main are instrumented.
 */
class FillInstrumentationGapsPhase : public EstimatorPhase {
 public:
  FillInstrumentationGapsPhase(metacg::Callgraph* callgraph);
  void modifyGraph(metacg::CgNode* mainMethod) override;

 protected:
  void printReport() override;

 private:
  struct NameComparator {
    bool operator()(const metacg::CgNode* lhs, const metacg::CgNode* rhs) const {
      return lhs->getFunctionName() < rhs->getFunctionName();
    }
  };
  std::set<metacg::CgNode*, NameComparator> nodesToFill;
  bool useCSInstrumentation;
  bool onlyEligibleNodes;
};

class StatisticsEstimatorPhase : public EstimatorPhase {
 public:
  StatisticsEstimatorPhase(bool shouldPrintReport, metacg::Callgraph* cg)
      : EstimatorPhase("StatisticsEstimatorPhase", cg),
        shouldPrintReport(shouldPrintReport),
        numFunctions(0),
        numReachableFunctions(0),
        totalStmts(0),
        stmtsCoveredWithInstr(0),
        stmtsActuallyCovered(0),
        totalVarDecls(0) {}

  void modifyGraph(metacg::CgNode* mainMethod) override;
  void printReport() override;
  long int getCuttoffNumInclStmts();
  long int getCuttoffReversesConditionalBranches() const;
  long int getCuttoffConditionalBranches() const;
  long int getCuttoffRoofline() const;
  long int getCuttoffLoopDepth() const;
  long int getCuttoffGlobalLoopDepth() const;

 private:
  using MapT = std::map<long int, long int>;
  long int getCuttoffValue(const MapT& hist) const;
  long int getUniqueMedianFromHist(const MapT& hist) const;
  long int getMedianFromHist(const MapT& hist) const;
  long int getHalfMaxFromHist(const MapT& hist) const;
  std::string printHist(const MapT& hist, const std::string& name);
  bool shouldPrintReport;
  long int numFunctions;
  long int numReachableFunctions;
  long int totalStmts;
  MapT stmtHist;
  MapT stmtInclHist;
  long int stmtsCoveredWithInstr;
  long int stmtsActuallyCovered;
  long int totalVarDecls;
  MapT conditionalBranchesInclHist;
  MapT reverseConditionalBranchesInclHist;
  MapT rooflineInclHist;
  MapT loopDepthInclHist;
  MapT globalLoopDepthInclHist;
};

/**
 * RN: An optimistic inclusive statement count heuristic.
 * Sums up statement count for all reachable nodes from a startNode.
 * Cycles are counted only once.
 * Edge counts are NOT taken into account.
 */
class StatementCountEstimatorPhase : public EstimatorPhase {
 public:
  explicit StatementCountEstimatorPhase(int numberOfStatementsThreshold, metacg::Callgraph* callgraph,
                                        bool inclusiveMetric = true, StatisticsEstimatorPhase* prevStatEP = nullptr);
  ~StatementCountEstimatorPhase() override;

  void modifyGraph(metacg::CgNode* mainMethod) override;
  void estimateStatementCount(metacg::CgNode* startNode, metacg::analysis::ReachabilityAnalysis& ra);

  int getNumStatements(metacg::CgNode* node) { return inclStmtCounts[node]; }

 private:
  int numberOfStatementsThreshold;
  bool inclusiveMetric;
  std::map<metacg::CgNode*, long int> inclStmtCounts;
  StatisticsEstimatorPhase* pSEP;
};

/**
 * @brief The RuntimeEstimatorPhase class
 */
class RuntimeEstimatorPhase : public EstimatorPhase {
 public:
  explicit RuntimeEstimatorPhase(metacg::Callgraph* cg, double runTimeThreshold, bool inclusiveMetric = true);
  ~RuntimeEstimatorPhase() override;

  void modifyGraph(metacg::CgNode* mainMethod) override;
  void estimateRuntime(metacg::CgNode* startNode);
  void doInstrumentation(metacg::CgNode* startNode, metacg::analysis::ReachabilityAnalysis& ra);

 private:
  // A hotspot is defines as a function with a runtime bigger than the threshold.
  // This is configured via the parameter file
  double thresholdHotspotInclusive = 0.2;
  double thresholdHotspotExclusive = 0.1;
  // Percentage of the overhead budget to atleast have for further exploration
  // This is configured via the parameter file
  double budgetForExploration = 0.3;
  double recursionFactor = 100;
  int mpiFunctionsStatementEstimate = 100;
  double inlineFactor = 100;

  /**
   * True if the node calls itself recursively, either directly or with one other node
   * @param node
   * @param cg
   * @return
   */
  static bool isSelfRecursive(metacg::CgNode* node, metacg::Callgraph* cg);
  static bool isLikelyToBeInlined(const metacg::CgNode* node, unsigned long stmtCount);

  struct NodeProfitComparator {
    bool operator()(const metacg::CgNode* lhs, const metacg::CgNode* rhs) const {
      const auto lhsNotKicked = !lhs->get<pira::TemporaryInstrumentationDecisionMetadata>()->isKicked;
      const auto rhsNotKicked = !rhs->get<pira::TemporaryInstrumentationDecisionMetadata>()->isKicked;
      const auto lhsPrevData = lhs->get<pira::InstrumentationResultMetaData>();
      const auto rhsPrevData = rhs->get<pira::InstrumentationResultMetaData>();
      const auto lhsTimePerCall =
          (lhsPrevData ? lhsPrevData->inclusiveTimePerCallSum : std::numeric_limits<double>::max());
      const auto rhsTimePerCall =
          (rhsPrevData ? rhsPrevData->inclusiveTimePerCallSum : std::numeric_limits<double>::max());
      const auto lhsInstroInfo = lhs->get<pira::TemporaryInstrumentationDecisionMetadata>()->info;
      const auto rhsInstroInfo = rhs->get<pira::TemporaryInstrumentationDecisionMetadata>()->info;
      const auto lhsParentHighRuntime =
          lhs->get<pira::TemporaryInstrumentationDecisionMetadata>()->parentHasHighExclusiveRuntime;
      const auto rhsParentHighRuntime =
          rhs->get<pira::TemporaryInstrumentationDecisionMetadata>()->parentHasHighExclusiveRuntime;

      // Priority for profit:
      // 1: Nodes with a higher PrevData are better than nodes without
      // 2: Nodes that are not kicked are better than kicked nodes
      // 3: Nodes where the parent has a high runtime are better than the ones where it has not
      // 4: Nodes that are not inlined are better than inlined nodes
      // 5: Nodes with a big Estimated win are better than nodes with a small
      return std::tie(lhsTimePerCall, lhsNotKicked, lhsParentHighRuntime, /*lhsNotInline,*/ lhsInstroInfo, lhs) >
             std::tie(rhsTimePerCall, rhsNotKicked, rhsParentHighRuntime, /*rhsNotInline,*/ rhsInstroInfo, rhs);
    }
  };

  /**
   * Compartor two compare the value of two nodes
   */
  struct NodeValueComparator {
    static bool compare(const metacg::CgNode* lhs, const metacg::CgNode* rhs) {
      assert(lhs);
      assert(rhs);
      const auto lhsNotKicked = !lhs->get<pira::TemporaryInstrumentationDecisionMetadata>()->isKicked;
      const auto rhsNotKicked = !rhs->get<pira::TemporaryInstrumentationDecisionMetadata>()->isKicked;
      const auto lhsPrevData = lhs->get<pira::InstrumentationResultMetaData>();
      const auto rhsPrevData = rhs->get<pira::InstrumentationResultMetaData>();
      const auto lhsTimePerCall =
          (lhsPrevData ? lhsPrevData->inclusiveTimePerCallSum : std::numeric_limits<double>::max());
      const auto rhsTimePerCall =
          (rhsPrevData ? rhsPrevData->inclusiveTimePerCallSum : std::numeric_limits<double>::max());
      const auto lhsInstroInfo = lhs->get<pira::TemporaryInstrumentationDecisionMetadata>()->info;
      const auto rhsInstroInfo = rhs->get<pira::TemporaryInstrumentationDecisionMetadata>()->info;
      const auto lhsParentHighRuntime =
          lhs->get<pira::TemporaryInstrumentationDecisionMetadata>()->parentHasHighExclusiveRuntime;
      const auto rhsParentHighRuntime =
          rhs->get<pira::TemporaryInstrumentationDecisionMetadata>()->parentHasHighExclusiveRuntime;

      // Priority for profit:
      // 1: Nodes with a higher PrevData are better than nodes without
      // 2: Nodes that are not kicked are better than kicked nodes
      // 3: Nodes where the parent has a high runtime are better than the ones where it has not
      // 4: Nodes that are not inlined are better than inlined nodes
      // 5: Nodes with a big large amount of statements are better
      return std::tie(lhsTimePerCall, lhsNotKicked, lhsParentHighRuntime,
                      /*lhsNotInline,*/ lhsInstroInfo.inclusiveStmtCount,
                      lhs) > std::tie(rhsTimePerCall, rhsNotKicked, rhsParentHighRuntime,
                                      /*rhsNotInline,*/ rhsInstroInfo.inclusiveStmtCount, rhs);
    }

    bool operator()(const metacg::CgNode* lhs, const metacg::CgNode* rhs) const { return compare(lhs, rhs); }
  };

  inline static double getCallsToNode(const metacg::CgNode* N) {
    const auto PrevInfo = N->get<pira::InstrumentationResultMetaData>();
    if (PrevInfo) {
      return PrevInfo->callCount;
    }
    return N->get<pira::TemporaryInstrumentationDecisionMetadata>()->info.callsFromParents;
  }

  std::pair<std::vector<metacg::CgNode*>, double> getNodesToInstrumentGreedyKnapsackOverhead(
      const std::set<metacg::CgNode*, NodeProfitComparator>& nodes, double costLimit);

  void modifyGraphOverhead(metacg::CgNode* mainMethod);

  // Calls, and inclusive stmts
  pira::InstumentationInfo getEstimatedInfoForInstrumentedNode(metacg::CgNode* node);

  // This only contains direct calls
  double getEstimatedCallCountForNode(metacg::CgNode* node, std::set<metacg::CgNode*>& blacklist);
  inline double getEstimatedCallCountForNode(metacg::CgNode* node, unsigned long stmtCount) {
    std::set<metacg::CgNode*> bl;
    bool isLikelyInlined = RuntimeEstimatorPhase::isLikelyToBeInlined(node, stmtCount);
    auto result = getEstimatedCallCountForNode(node, bl);
    assert(bl.empty());
    if (isLikelyInlined) {
      // TODO: Maybe special handling for trait classes
      metacg::MCGLogger::instance().getConsole()->debug("Applying inline factor of {} for node {}", inlineFactor,
                                                        node->getFunctionName());
      result *= inlineFactor;
    }
    return result;
  }

  double runTimeThreshold;
  bool inclusiveMetric;
  std::map<metacg::CgNode*, double> inclRunTime;
  double relativeOverhead;

  unsigned long long int totalExclusiveCalls = 0;
  std::set<std::pair<unsigned long long int, metacg::CgNode*>, std::greater<>> exclusiveCalls;

  std::set<metacg::CgNode*> kickList;
  bool useCSInstrumentation;
  bool onlyEligibleNodes;
  /**
   * Kick nodes based on their inclusive runtime per call, prioritize non hotspot nodes
   * @param mainMethod
   * @param nodesSortedByRuntimePerCallNoHotspot
   * @param nodesSortedByRuntimePerCallHotspot
   * @param callsToKick
   * @return
   */
  double kickNodesByRuntimePerCall(const metacg::CgNode* mainMethod,
                                   std::vector<metacg::CgNode*>& nodesSortedByRuntimePerCallNoHotspot,
                                   std::vector<metacg::CgNode*>& nodesSortedByRuntimePerCallHotspot,
                                   double callsToKick) const;
  /**
   * Kick leaf nodes randomly, prioritize non hotspot nodes
   * @param mainMethod
   * @param nodesNoHotspot
   * @param nodesHotspot
   * @param callsToKick
   * @return
   */
  double kickNodesRandomly(const metacg::CgNode* mainMethod, std::vector<metacg::CgNode*>& nodesNoHotspot,
                           std::vector<metacg::CgNode*>& nodesHotspot, double callsToKick) const;
  double kickNodesByRuntimePerCallKeepSmall(const metacg::CgNode* mainMethod,
                                            std::vector<metacg::CgNode*>& nodesSortedByRuntimePerCallNoHotspot,
                                            std::vector<metacg::CgNode*>& nodesSortedByRuntimePerCallHotspot,
                                            const double callsToKick) const;
  void kickNodesFromInstrumentation(const metacg::CgNode* mainMethod, const std::vector<metacg::CgNode*>& nodesToKick,
                                    const double callsToKick, double& kicked) const;
  void kickSingleNode(metacg::CgNode* node, double& kicked) const;
};

/**
 * RN: Gets a file with a whitelist of interesting nodes.
 * Instruments all paths to these nodes with naive callpathDifferentiation.
 */
class WLCallpathDifferentiationEstimatorPhase : public EstimatorPhase {
 public:
  explicit WLCallpathDifferentiationEstimatorPhase(std::string whiteListName = "whitelist.txt");
  ~WLCallpathDifferentiationEstimatorPhase() override;

  void modifyGraph(metacg::CgNode* mainMethod) override;

 private:
  metacg::CgNodeRawPtrUSet whitelist;  // all whitelisted nodes INCL. their paths to main
  std::string whitelistName;

  void addNodeAndParentsToWhitelist(metacg::CgNode* node);
};

class SummingCountPhaseBase : public EstimatorPhase {
 public:
  SummingCountPhaseBase(long int threshold, const std::string& name, metacg::Callgraph* callgraph,
                        StatisticsEstimatorPhase* prevStatEP, bool inclusive = true);
  ~SummingCountPhaseBase() override;
  void modifyGraph(metacg::CgNode* mainMethod) override;
  static const long int limitThreshold = std::numeric_limits<long int>::max();
  long int getCounted(const metacg::CgNode* node);

 protected:
  void estimateCount(metacg::CgNode* startNode, metacg::analysis::ReachabilityAnalysis& ra);
  virtual long int getPreviousThreshold() const = 0;
  virtual long int getTargetCount(const metacg::CgNode* node) const = 0;
  long int threshold;
  std::map<const metacg::CgNode*, long int> counts;
  StatisticsEstimatorPhase* pSEP;
  virtual void runInitialization();
  const bool inclusive;
};

// Inclusive count
class ConditionalBranchesEstimatorPhase : public SummingCountPhaseBase {
 public:
  explicit ConditionalBranchesEstimatorPhase(long int threshold, metacg::Callgraph* callgraph,
                                             StatisticsEstimatorPhase* prevStatEP = nullptr);

 protected:
  long int getPreviousThreshold() const override;
  long int getTargetCount(const metacg::CgNode* node) const override;
};

// Calculates the target count by subtracting the conditional branches from the max amount of conditional branches
// Inclusive count
class ConditionalBranchesReverseEstimatorPhase : public SummingCountPhaseBase {
 public:
  explicit ConditionalBranchesReverseEstimatorPhase(long int threshold, metacg::Callgraph* callgraph,
                                                    StatisticsEstimatorPhase* prevStatEP = nullptr);

 protected:
  long int maxBranches = 0;
  long int getPreviousThreshold() const override;
  long int getTargetCount(const metacg::CgNode* node) const override;
  void runInitialization() override;
};

// Uses the combined Number of Floating-Point Operations and memory accesses
// Inclusive count
class FPAndMemOpsEstimatorPhase : public SummingCountPhaseBase {
 public:
  explicit FPAndMemOpsEstimatorPhase(long int threshold, metacg::Callgraph* callgraph,
                                     StatisticsEstimatorPhase* prevStatEP = nullptr);

 protected:
  long int getPreviousThreshold() const override;
  long int getTargetCount(const metacg::CgNode* node) const override;
};

// Exclusive count
class LoopDepthEstimatorPhase : public SummingCountPhaseBase {
 public:
  explicit LoopDepthEstimatorPhase(long int threshold, metacg::Callgraph* callgraph,
                                   StatisticsEstimatorPhase* prevStatEP = nullptr);

 protected:
  long int getPreviousThreshold() const override;
  long int getTargetCount(const metacg::CgNode* node) const override;
};

// Exclusive count
class GlobalLoopDepthEstimatorPhase : public SummingCountPhaseBase {
 public:
  explicit GlobalLoopDepthEstimatorPhase(long int threshold, metacg::Callgraph* callgraph,
                                         StatisticsEstimatorPhase* prevStatEP = nullptr);

 protected:
  long int getPreviousThreshold() const override;
  long int getTargetCount(const metacg::CgNode* node) const override;
};

/**
 * Sets the InstrumentationResultMetaData.shouldBeInstrumented to true if a function should be instrumented.
 * If a function does not have the metadata, it gets attached if the function should be instrumented.
 * If it should be not instrumented, only existing metadata gets modified to "false" and no new metadata gets added.
 *
 * This pass is only run in PIRA I mode and only used by the overhead based heuristic.
 */
class StoreInstrumentationDecisionsPhase : public EstimatorPhase {
 public:
  explicit StoreInstrumentationDecisionsPhase(metacg::Callgraph* callgraph);
  void modifyGraph(metacg::CgNode* /*mainMethod*/) override;
  ~StoreInstrumentationDecisionsPhase() override = default;
};

#endif

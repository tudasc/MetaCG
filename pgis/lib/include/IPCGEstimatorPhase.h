/**
 * File: IPCGEstimatorPhase.h
 * License: Part of the MetaCG project. Licensed under BSD 3 clause license. See LICENSE.txt file at
 * https://github.com/tudasc/metacg/LICENSE.txt
 */

#ifndef IPCGESTIMATORPHASE_H_
#define IPCGESTIMATORPHASE_H_

#include "EstimatorPhase.h"

#include <map>
#include <queue>
#include <set>

/** RN: instrument the first n levels starting from main */
class FirstNLevelsEstimatorPhase : public EstimatorPhase {
 public:
  FirstNLevelsEstimatorPhase(int levels);
  ~FirstNLevelsEstimatorPhase();

  void modifyGraph(CgNodePtr mainMethod);

 private:
  void instrumentLevel(CgNodePtr parentNode, int levelsLeft);

  const int levels;
};

class StatisticsEstimatorPhase : public EstimatorPhase {
 public:
  StatisticsEstimatorPhase(bool shouldPrintReport)
      : EstimatorPhase("StatisticsEstimatorPhase"),
        shouldPrintReport(shouldPrintReport),
        numFunctions(0),
        numReachableFunctions(0),
        totalStmts(0),
        stmtsCoveredWithInstr(0),
        stmtsActuallyCovered(0),
        totalVarDecls(0) {}

  void modifyGraph(CgNodePtr mainMethod) override;
  void printReport() override;
  long int getMedianNumInclStmts();
  long int getMaxNumInclStmts();

 private:
  bool shouldPrintReport;
  long int numFunctions;
  long int numReachableFunctions;
  long int totalStmts;
  std::map<long int, long int> stmtHist;
  std::map<long int, long int> stmtInclHist;
  long int stmtsCoveredWithInstr;
  long int stmtsActuallyCovered;
  long int totalVarDecls;
};

/**
 * RN: An optimistic inclusive statement count heuristic.
 * Sums up statement count for all reachable nodes from a startNode.
 * Cycles are counted only once.
 * Edge counts are NOT taken into account.
 */
class StatementCountEstimatorPhase : public EstimatorPhase {
 public:
  StatementCountEstimatorPhase(int numberOfStatementsThreshold, bool inclusiveMetric = true,
                               StatisticsEstimatorPhase *prevStatEP = nullptr);
  ~StatementCountEstimatorPhase();

  void modifyGraph(CgNodePtr mainMethod) override;
  void estimateStatementCount(CgNodePtr startNode);

  int getNumStatements(CgNodePtr node) { return inclStmtCounts[node]; }

 private:
  int numberOfStatementsThreshold;
  bool inclusiveMetric;
  std::map<CgNodePtr, long int> inclStmtCounts;
  StatisticsEstimatorPhase *pSEP;
};

/**
 * @brief The HybridSelectionStrategy base class
 */
class HybridSelectionStrategy {
 public:
  virtual void operator()(CgNodePtr node) = 0;
};

struct MaxRuntimeSelectionStrategy : public HybridSelectionStrategy {
  void operator()(CgNodePtr node) override;
};

struct MaxStmtSelectionStrategy : public HybridSelectionStrategy {
  void operator()(CgNodePtr node) override;
};

struct RuntimeFilteredMixedStrategy : public HybridSelectionStrategy {
  RuntimeFilteredMixedStrategy(long stmtThreshold = 0, double rtThreshold = .0)
      : stmtThresh(stmtThreshold), rtThresh(rtThreshold) {}
  void operator()(CgNodePtr node) override;
  long stmtThresh;
  double rtThresh;
};

/**
 * @brief The HybridEstimatorPhase class
 * This is the playground for hybrid selection strategies.
 */
class HybridEstimatorPhase : public EstimatorPhase {
 public:
  HybridEstimatorPhase() : EstimatorPhase("HybridEstimatorPhase") {}

  void modifyGraph(CgNodePtr node) override;
  void addStrategy(HybridSelectionStrategy *strat) { strategies.push_back(strat); }

 private:
  std::vector<HybridSelectionStrategy *> strategies;
};

/**
 * @brief The RuntimeEstimatorPhase class
 */
class RuntimeEstimatorPhase : public EstimatorPhase {
 public:
  RuntimeEstimatorPhase(double runTimeThreshold, bool inclusiveMetric = true);
  ~RuntimeEstimatorPhase();

  void modifyGraph(CgNodePtr mainMethod) override;
  void estimateRuntime(CgNodePtr startNode);
  void doInstrumentation(CgNodePtr startNode);

 private:
  double runTimeThreshold;
  bool inclusiveMetric;
  std::map<CgNodePtr, double> inclRunTime;
};

/**
 * RN: Gets a file with a whitelist of interesting nodes.
 * Instruments all paths to these nodes with naive callpathDifferentiation.
 */
class WLCallpathDifferentiationEstimatorPhase : public EstimatorPhase {
 public:
  WLCallpathDifferentiationEstimatorPhase(std::string whiteListName = "whitelist.txt");
  ~WLCallpathDifferentiationEstimatorPhase();

  void modifyGraph(CgNodePtr mainMethod) override;

 private:
  CgNodePtrSet whitelist;  // all whitelisted nodes INCL. their paths to main
  std::string whitelistName;

  void addNodeAndParentsToWhitelist(CgNodePtr node);
};

#endif

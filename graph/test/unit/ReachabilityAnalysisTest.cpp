/**
 * File: ReachabilityAnalysisTest.cpp
 * License: Part of the metacg project. Licensed under BSD 3 clause license. See LICENSE.txt file at
 * https://github.com/tudasc/metacg/LICENSE.txt
 */

#include "gtest/gtest.h"

#include "LoggerUtil.h"
#include "MCGManager.h"
#include "ReachabilityAnalysis.h"

namespace {
using namespace metacg::analysis;

static const char* const mainS = "main";
static const char* const LC1 = "leftChild1";
static const char* const RC1 = "rightChild1";
static const char* const LC2 = "leftChild2";
static const char* const RC2 = "rightChild2";
static const char* const LC3 = "leftChild3";
static const char* const RC3 = "rightChild3";
static const char* const LC4 = "leftChild4";
static const char* const RC4 = "rightChild4";

class ReachabilityAnalysisTest : public ::testing::Test {
 protected:
  void SetUp() override {
    metacg::loggerutil::getLogger();
    auto& mcgm = metacg::graph::MCGManager::get();
    mcgm.resetManager();
    mcgm.addToManagedGraphs("testgraph", std::make_unique<metacg::Callgraph>());
  }

  metacg::Callgraph* getGraph() {
    auto& mcgm = metacg::graph::MCGManager::get();
    return mcgm.getCallgraph();
  }

  void fillGraph(metacg::Callgraph* graph) {
    graph->getOrInsertNode(mainS);
    graph->getOrInsertNode(LC1);
    graph->getOrInsertNode(RC1);
    graph->getOrInsertNode(LC2);
    graph->getOrInsertNode(RC2);
    graph->getOrInsertNode(LC3);
    graph->getOrInsertNode(RC3);
    graph->getOrInsertNode(LC4);
    graph->getOrInsertNode(RC4);
  }
};

TEST_F(ReachabilityAnalysisTest, NoneReachable) {
  auto cg = getGraph();
  ASSERT_TRUE(cg != nullptr);
  fillGraph(cg);
  ReachabilityAnalysis ra(cg);
  ASSERT_FALSE(ra.existsPathBetween(cg->getNode(mainS), cg->getNode(LC1)));
  ASSERT_FALSE(ra.existsPathBetween(cg->getNode(mainS), cg->getNode(LC2)));
  ASSERT_FALSE(ra.existsPathBetween(cg->getNode(mainS), cg->getNode(LC3)));
  ASSERT_FALSE(ra.existsPathBetween(cg->getNode(mainS), cg->getNode(LC4)));

  ASSERT_FALSE(ra.existsPathBetween(cg->getNode(mainS), cg->getNode(RC1)));
  ASSERT_FALSE(ra.existsPathBetween(cg->getNode(mainS), cg->getNode(RC2)));
  ASSERT_FALSE(ra.existsPathBetween(cg->getNode(mainS), cg->getNode(RC3)));
  ASSERT_FALSE(ra.existsPathBetween(cg->getNode(mainS), cg->getNode(RC4)));
}

TEST_F(ReachabilityAnalysisTest, OneReachable) {
  auto cg = getGraph();
  ASSERT_TRUE(cg != nullptr);
  fillGraph(cg);
  cg->addEdge(mainS, LC1);
  // Explicit call to compute the result
  ReachabilityAnalysis ra(cg);
  ra.computeReachableFromMain();
  ASSERT_TRUE(ra.isReachableFromMain(cg->getNode(LC1)));
  ASSERT_FALSE(ra.isReachableFromMain(cg->getNode(LC2)));
  ASSERT_FALSE(ra.isReachableFromMain(cg->getNode(LC3)));
  ASSERT_FALSE(ra.isReachableFromMain(cg->getNode(LC4)));
  ASSERT_FALSE(ra.isReachableFromMain(cg->getNode(RC1)));
  ASSERT_FALSE(ra.isReachableFromMain(cg->getNode(RC2)));
  ASSERT_FALSE(ra.isReachableFromMain(cg->getNode(RC3)));
  ASSERT_FALSE(ra.isReachableFromMain(cg->getNode(RC4)));

  // No Explicit call to compute the result, yet implicitly triggered
  ReachabilityAnalysis ra2(cg);
  ASSERT_TRUE(ra2.isReachableFromMain(cg->getNode(LC1)));
  ASSERT_FALSE(ra2.isReachableFromMain(cg->getNode(LC2)));
  ASSERT_FALSE(ra2.isReachableFromMain(cg->getNode(LC3)));
  ASSERT_FALSE(ra2.isReachableFromMain(cg->getNode(LC4)));
  ASSERT_FALSE(ra2.isReachableFromMain(cg->getNode(RC1)));
  ASSERT_FALSE(ra2.isReachableFromMain(cg->getNode(RC2)));
  ASSERT_FALSE(ra2.isReachableFromMain(cg->getNode(RC3)));
  ASSERT_FALSE(ra2.isReachableFromMain(cg->getNode(RC4)));

  // No explicit call, but force RA to recompute reachable set
  ReachabilityAnalysis ra3(cg);
  ASSERT_TRUE(ra3.isReachableFromMain(cg->getNode(LC1), true));
  ASSERT_FALSE(ra3.isReachableFromMain(cg->getNode(LC2)));
  ASSERT_FALSE(ra3.isReachableFromMain(cg->getNode(LC3)));
  ASSERT_FALSE(ra3.isReachableFromMain(cg->getNode(LC4)));
  ASSERT_FALSE(ra3.isReachableFromMain(cg->getNode(RC1)));
  ASSERT_FALSE(ra3.isReachableFromMain(cg->getNode(RC2)));
  ASSERT_FALSE(ra3.isReachableFromMain(cg->getNode(RC3)));
  ASSERT_FALSE(ra3.isReachableFromMain(cg->getNode(RC4)));
}

TEST_F(ReachabilityAnalysisTest, TwoReachable) {
  auto cg = getGraph();
  ASSERT_TRUE(cg != nullptr);
  fillGraph(cg);
  cg->addEdge(mainS, LC1);
  cg->addEdge(mainS, LC2);
  // Explicit call to compute the result
  ReachabilityAnalysis ra(cg);
  ra.computeReachableFromMain();
  ASSERT_TRUE(ra.isReachableFromMain(cg->getNode(LC1)));
  ASSERT_TRUE(ra.isReachableFromMain(cg->getNode(LC2)));
  ASSERT_FALSE(ra.isReachableFromMain(cg->getNode(LC3)));
  ASSERT_FALSE(ra.isReachableFromMain(cg->getNode(LC4)));
  ASSERT_FALSE(ra.isReachableFromMain(cg->getNode(RC1)));
  ASSERT_FALSE(ra.isReachableFromMain(cg->getNode(RC2)));
  ASSERT_FALSE(ra.isReachableFromMain(cg->getNode(RC3)));
  ASSERT_FALSE(ra.isReachableFromMain(cg->getNode(RC4)));

  // No Explicit call to compute the result, yet implicitly triggered
  ReachabilityAnalysis ra2(cg);
  ASSERT_TRUE(ra2.isReachableFromMain(cg->getNode(LC1)));
  ASSERT_TRUE(ra2.isReachableFromMain(cg->getNode(LC2)));
  ASSERT_FALSE(ra2.isReachableFromMain(cg->getNode(LC3)));
  ASSERT_FALSE(ra2.isReachableFromMain(cg->getNode(LC4)));
  ASSERT_FALSE(ra2.isReachableFromMain(cg->getNode(RC1)));
  ASSERT_FALSE(ra2.isReachableFromMain(cg->getNode(RC2)));
  ASSERT_FALSE(ra2.isReachableFromMain(cg->getNode(RC3)));
  ASSERT_FALSE(ra2.isReachableFromMain(cg->getNode(RC4)));

  // No explicit call, but force RA to recompute reachable set
  ReachabilityAnalysis ra3(cg);
  ASSERT_TRUE(ra3.isReachableFromMain(cg->getNode(LC1), true));
  ASSERT_TRUE(ra3.isReachableFromMain(cg->getNode(LC2), true));
  ASSERT_FALSE(ra3.isReachableFromMain(cg->getNode(LC3)));
  ASSERT_FALSE(ra3.isReachableFromMain(cg->getNode(LC4)));
  ASSERT_FALSE(ra3.isReachableFromMain(cg->getNode(RC1)));
  ASSERT_FALSE(ra3.isReachableFromMain(cg->getNode(RC2)));
  ASSERT_FALSE(ra3.isReachableFromMain(cg->getNode(RC3)));
  ASSERT_FALSE(ra3.isReachableFromMain(cg->getNode(RC4)));
}

TEST_F(ReachabilityAnalysisTest, ReachableBetweenTwoNodes) {
  auto cg = getGraph();
  ASSERT_TRUE(cg != nullptr);
  fillGraph(cg);
  cg->addEdge(mainS, LC1);
  cg->addEdge(mainS, LC2);
  cg->addEdge(LC2, RC1);
  ReachabilityAnalysis ra(cg);
  ASSERT_TRUE(ra.existsPathBetween(cg->getNode(mainS), cg->getNode(LC1)));
  ASSERT_TRUE(ra.existsPathBetween(cg->getNode(LC2), cg->getNode(RC1)));
  ASSERT_FALSE(ra.existsPathBetween(cg->getNode(LC4), cg->getNode(RC3)));
}

TEST_F(ReachabilityAnalysisTest, ReachableBetweenNodesNoCycle) {
  auto cg = getGraph();
  ASSERT_TRUE(cg != nullptr);
  fillGraph(cg);
  cg->addEdge(mainS, LC1);
  cg->addEdge(mainS, LC2);
  cg->addEdge(LC2, RC1);
  cg->addEdge(RC1, RC2);
  ReachabilityAnalysis ra(cg);
  ASSERT_TRUE(ra.existsPathBetween(cg->getNode(mainS), cg->getNode(RC2)));
  ASSERT_TRUE(ra.existsPathBetween(cg->getNode(LC2), cg->getNode(RC2)));
  ASSERT_FALSE(ra.existsPathBetween(cg->getNode(RC2), cg->getNode(RC3)));
}

TEST_F(ReachabilityAnalysisTest, ReachableBetweenNodesWithCycle) {
  auto cg = getGraph();
  ASSERT_TRUE(cg != nullptr);
  fillGraph(cg);
  cg->addEdge(mainS, LC1);
  cg->addEdge(LC1, RC1);
  cg->addEdge(RC1, RC2);
  cg->addEdge(RC2, LC1);  // creates cycle main -> lc1 -> rc1 -> rc2 -> lc1 -> lc2
  cg->addEdge(LC1, LC2);
  cg->addEdge(mainS, LC2);
  ReachabilityAnalysis ra(cg);
  ASSERT_TRUE(ra.existsPathBetween(cg->getNode(mainS), cg->getNode(LC2)));
  ASSERT_TRUE(ra.existsPathBetween(cg->getNode(RC2), cg->getNode(LC2)));
  ASSERT_FALSE(ra.existsPathBetween(cg->getNode(RC2), cg->getNode(RC3)));
}
}  // namespace

/**
 * File: ReachabilityAnalysisTest.cpp
 * License: Part of the metacg project. Licensed under BSD 3 clause license. See LICENSE.txt file at
 * https://github.com/tudasc/metacg/LICENSE.txt
 */

#include "gtest/gtest.h"

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
    graph->insert(mainS);
    graph->insert(LC1);
    graph->insert(RC1);
    graph->insert(LC2);
    graph->insert(RC2);
    graph->insert(LC3);
    graph->insert(RC3);
    graph->insert(LC4);
    graph->insert(RC4);
  }
};

TEST_F(ReachabilityAnalysisTest, NoneReachable) {
  auto cg = getGraph();
  ASSERT_TRUE(cg != nullptr);
  fillGraph(cg);
  ReachabilityAnalysis ra(cg);
  ASSERT_FALSE(ra.existsPathBetween(cg->getFirstNode(mainS), cg->getFirstNode(LC1)));
  ASSERT_FALSE(ra.existsPathBetween(cg->getFirstNode(mainS), cg->getFirstNode(LC2)));
  ASSERT_FALSE(ra.existsPathBetween(cg->getFirstNode(mainS), cg->getFirstNode(LC3)));
  ASSERT_FALSE(ra.existsPathBetween(cg->getFirstNode(mainS), cg->getFirstNode(LC4)));

  ASSERT_FALSE(ra.existsPathBetween(cg->getFirstNode(mainS), cg->getFirstNode(RC1)));
  ASSERT_FALSE(ra.existsPathBetween(cg->getFirstNode(mainS), cg->getFirstNode(RC2)));
  ASSERT_FALSE(ra.existsPathBetween(cg->getFirstNode(mainS), cg->getFirstNode(RC3)));
  ASSERT_FALSE(ra.existsPathBetween(cg->getFirstNode(mainS), cg->getFirstNode(RC4)));
}

TEST_F(ReachabilityAnalysisTest, OneReachable) {
  auto cg = getGraph();
  ASSERT_TRUE(cg != nullptr);
  fillGraph(cg);
  ASSERT_TRUE(cg->addEdge(mainS, LC1));
  // Explicit call to compute the result
  ReachabilityAnalysis ra(cg);
  ra.computeReachableFromMain();
  ASSERT_TRUE(ra.isReachableFromMain(cg->getFirstNode(LC1)));
  ASSERT_FALSE(ra.isReachableFromMain(cg->getFirstNode(LC2)));
  ASSERT_FALSE(ra.isReachableFromMain(cg->getFirstNode(LC3)));
  ASSERT_FALSE(ra.isReachableFromMain(cg->getFirstNode(LC4)));
  ASSERT_FALSE(ra.isReachableFromMain(cg->getFirstNode(RC1)));
  ASSERT_FALSE(ra.isReachableFromMain(cg->getFirstNode(RC2)));
  ASSERT_FALSE(ra.isReachableFromMain(cg->getFirstNode(RC3)));
  ASSERT_FALSE(ra.isReachableFromMain(cg->getFirstNode(RC4)));

  // No Explicit call to compute the result, yet implicitly triggered
  ReachabilityAnalysis ra2(cg);
  ASSERT_TRUE(ra2.isReachableFromMain(cg->getFirstNode(LC1)));
  ASSERT_FALSE(ra2.isReachableFromMain(cg->getFirstNode(LC2)));
  ASSERT_FALSE(ra2.isReachableFromMain(cg->getFirstNode(LC3)));
  ASSERT_FALSE(ra2.isReachableFromMain(cg->getFirstNode(LC4)));
  ASSERT_FALSE(ra2.isReachableFromMain(cg->getFirstNode(RC1)));
  ASSERT_FALSE(ra2.isReachableFromMain(cg->getFirstNode(RC2)));
  ASSERT_FALSE(ra2.isReachableFromMain(cg->getFirstNode(RC3)));
  ASSERT_FALSE(ra2.isReachableFromMain(cg->getFirstNode(RC4)));

  // No explicit call, but force RA to recompute reachable set
  ReachabilityAnalysis ra3(cg);
  ASSERT_TRUE(ra3.isReachableFromMain(cg->getFirstNode(LC1), true));
  ASSERT_FALSE(ra3.isReachableFromMain(cg->getFirstNode(LC2)));
  ASSERT_FALSE(ra3.isReachableFromMain(cg->getFirstNode(LC3)));
  ASSERT_FALSE(ra3.isReachableFromMain(cg->getFirstNode(LC4)));
  ASSERT_FALSE(ra3.isReachableFromMain(cg->getFirstNode(RC1)));
  ASSERT_FALSE(ra3.isReachableFromMain(cg->getFirstNode(RC2)));
  ASSERT_FALSE(ra3.isReachableFromMain(cg->getFirstNode(RC3)));
  ASSERT_FALSE(ra3.isReachableFromMain(cg->getFirstNode(RC4)));
}

TEST_F(ReachabilityAnalysisTest, TwoReachable) {
  auto cg = getGraph();
  ASSERT_TRUE(cg != nullptr);
  fillGraph(cg);
  ASSERT_TRUE(cg->addEdge(mainS, LC1));
  ASSERT_TRUE(cg->addEdge(mainS, LC2));
  // Explicit call to compute the result
  ReachabilityAnalysis ra(cg);
  ra.computeReachableFromMain();
  ASSERT_TRUE(ra.isReachableFromMain(cg->getFirstNode(LC1)));
  ASSERT_TRUE(ra.isReachableFromMain(cg->getFirstNode(LC2)));
  ASSERT_FALSE(ra.isReachableFromMain(cg->getFirstNode(LC3)));
  ASSERT_FALSE(ra.isReachableFromMain(cg->getFirstNode(LC4)));
  ASSERT_FALSE(ra.isReachableFromMain(cg->getFirstNode(RC1)));
  ASSERT_FALSE(ra.isReachableFromMain(cg->getFirstNode(RC2)));
  ASSERT_FALSE(ra.isReachableFromMain(cg->getFirstNode(RC3)));
  ASSERT_FALSE(ra.isReachableFromMain(cg->getFirstNode(RC4)));

  // No Explicit call to compute the result, yet implicitly triggered
  ReachabilityAnalysis ra2(cg);
  ASSERT_TRUE(ra2.isReachableFromMain(cg->getFirstNode(LC1)));
  ASSERT_TRUE(ra2.isReachableFromMain(cg->getFirstNode(LC2)));
  ASSERT_FALSE(ra2.isReachableFromMain(cg->getFirstNode(LC3)));
  ASSERT_FALSE(ra2.isReachableFromMain(cg->getFirstNode(LC4)));
  ASSERT_FALSE(ra2.isReachableFromMain(cg->getFirstNode(RC1)));
  ASSERT_FALSE(ra2.isReachableFromMain(cg->getFirstNode(RC2)));
  ASSERT_FALSE(ra2.isReachableFromMain(cg->getFirstNode(RC3)));
  ASSERT_FALSE(ra2.isReachableFromMain(cg->getFirstNode(RC4)));

  // No explicit call, but force RA to recompute reachable set
  ReachabilityAnalysis ra3(cg);
  ASSERT_TRUE(ra3.isReachableFromMain(cg->getFirstNode(LC1), true));
  ASSERT_TRUE(ra3.isReachableFromMain(cg->getFirstNode(LC2), true));
  ASSERT_FALSE(ra3.isReachableFromMain(cg->getFirstNode(LC3)));
  ASSERT_FALSE(ra3.isReachableFromMain(cg->getFirstNode(LC4)));
  ASSERT_FALSE(ra3.isReachableFromMain(cg->getFirstNode(RC1)));
  ASSERT_FALSE(ra3.isReachableFromMain(cg->getFirstNode(RC2)));
  ASSERT_FALSE(ra3.isReachableFromMain(cg->getFirstNode(RC3)));
  ASSERT_FALSE(ra3.isReachableFromMain(cg->getFirstNode(RC4)));
}

TEST_F(ReachabilityAnalysisTest, ReachableBetweenTwoNodes) {
  auto cg = getGraph();
  ASSERT_TRUE(cg != nullptr);
  fillGraph(cg);
  ASSERT_TRUE(cg->addEdge(mainS, LC1));
  ASSERT_TRUE(cg->addEdge(mainS, LC2));
  ASSERT_TRUE(cg->addEdge(LC2, RC1));
  ReachabilityAnalysis ra(cg);
  ASSERT_TRUE(ra.existsPathBetween(cg->getFirstNode(mainS), cg->getFirstNode(LC1)));
  ASSERT_TRUE(ra.existsPathBetween(cg->getFirstNode(LC2), cg->getFirstNode(RC1)));
  ASSERT_FALSE(ra.existsPathBetween(cg->getFirstNode(LC4), cg->getFirstNode(RC3)));
}

TEST_F(ReachabilityAnalysisTest, ReachableBetweenNodesNoCycle) {
  auto cg = getGraph();
  ASSERT_TRUE(cg != nullptr);
  fillGraph(cg);
  ASSERT_TRUE(cg->addEdge(mainS, LC1));
  ASSERT_TRUE(cg->addEdge(mainS, LC2));
  ASSERT_TRUE(cg->addEdge(LC2, RC1));
  ASSERT_TRUE(cg->addEdge(RC1, RC2));
  ReachabilityAnalysis ra(cg);
  ASSERT_TRUE(ra.existsPathBetween(cg->getFirstNode(mainS), cg->getFirstNode(RC2)));
  ASSERT_TRUE(ra.existsPathBetween(cg->getFirstNode(LC2), cg->getFirstNode(RC2)));
  ASSERT_FALSE(ra.existsPathBetween(cg->getFirstNode(RC2), cg->getFirstNode(RC3)));
}

TEST_F(ReachabilityAnalysisTest, ReachableBetweenNodesWithCycle) {
  auto cg = getGraph();
  ASSERT_TRUE(cg != nullptr);
  fillGraph(cg);
  ASSERT_TRUE(cg->addEdge(mainS, LC1));
  ASSERT_TRUE(cg->addEdge(LC1, RC1));
  ASSERT_TRUE(cg->addEdge(RC1, RC2));
  ASSERT_TRUE(cg->addEdge(RC2, LC1));  // creates cycle main -> lc1 -> rc1 -> rc2 -> lc1 -> lc2
  ASSERT_TRUE(cg->addEdge(LC1, LC2));
  ASSERT_TRUE(cg->addEdge(mainS, LC2));
  ReachabilityAnalysis ra(cg);
  ASSERT_TRUE(ra.existsPathBetween(cg->getFirstNode(mainS), cg->getFirstNode(LC2)));
  ASSERT_TRUE(ra.existsPathBetween(cg->getFirstNode(RC2), cg->getFirstNode(LC2)));
  ASSERT_FALSE(ra.existsPathBetween(cg->getFirstNode(RC2), cg->getFirstNode(RC3)));
}
}  // namespace

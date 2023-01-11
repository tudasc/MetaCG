/**
 * File: CallgraphManagerTest.cpp
 * License: Part of the metacg project. Licensed under BSD 3 clause license. See LICENSE.txt file at
 * https://github.com/tudasc/metacg/LICENSE.txt
 */

#include "gtest/gtest.h"

#include "LoggerUtil.h"

#include "PiraMCGProcessor.h"

using namespace pira;

class CallgraphManagerTest : public ::testing::Test {
 protected:
  void SetUp() override { loggerutil::getLogger(); }
};

TEST_F(CallgraphManagerTest, EmptyCG) {
  Config cfg;
  // PiraMCGProcessor cm(&cfg);
  auto &cm = metacg::pgis::PiraMCGProcessor::get();
  cm.setConfig(&cfg);
  ASSERT_EQ(0, cm.size());
  auto graph = cm.getCallgraph(&cm);
  ASSERT_EQ(false, graph.hasNode("main"));
  ASSERT_EQ(nullptr, graph.findMain());
  ASSERT_EQ(0, graph.size());
}

TEST_F(CallgraphManagerTest, OneNodeCG) {
  Config cfg;
  // PiraMCGProcessor cm(&cfg);
  auto &cm = metacg::pgis::PiraMCGProcessor::get();
  cm.clear();
  cm.setConfig(&cfg);
  cm.getCallgraph()->getOrInsertNode("main");
  cm.setNodeComesFromCube("main");
  auto nPtr = cm.getCallgraph()->getOrInsertNode("main");
  auto graph = cm.getCallgraph(&cm);
  ASSERT_NE(nullptr, graph.findMain());
  ASSERT_EQ(nPtr, graph.findMain());
  ASSERT_TRUE(graph.findMain()->get<BaseProfileData>());
  ASSERT_TRUE(graph.findMain()->get<PiraOneData>());
  ASSERT_TRUE(nPtr->get<BaseProfileData>());
  ASSERT_TRUE(nPtr->get<PiraOneData>());
  ASSERT_EQ(true, nPtr->get<PiraOneData>()->comesFromCube());
}

TEST_F(CallgraphManagerTest, TwoNodeCG) {
  Config cfg;
  // PiraMCGProcessor cm(&cfg);
  auto &cm = metacg::pgis::PiraMCGProcessor::get();
  cm.setConfig(&cfg);
  int mainLineNumber = 1;
  auto mainNode = cm.getCallgraph()->getOrInsertNode("main");
  auto childNode = cm.getCallgraph()->getOrInsertNode("child1");
  cm.putEdge("main", "fileA.c", mainLineNumber, "child1", 10, 1, 0, 0);
  ASSERT_EQ(mainNode, cm.getCallgraph()->getOrInsertNode("main"));
  ASSERT_EQ(childNode, cm.getCallgraph()->getOrInsertNode("child1"));
  auto graph = cm.getCallgraph(&cm);
  ASSERT_EQ(mainNode, graph.findMain());
  ASSERT_EQ(childNode, graph.findNode("child1"));
  auto mainNode_f = graph.findMain();
  ASSERT_EQ(mainLineNumber, mainNode->getLineNumber());
}

TEST_F(CallgraphManagerTest, ThreeNodeCG) {
  Config cfg;
  // PiraMCGProcessor cm(&cfg);
  auto &cm = metacg::pgis::PiraMCGProcessor::get();
  cm.setConfig(&cfg);
  int mainLineNumber = 1;
  auto mainNode = cm.getCallgraph()->getOrInsertNode("main");
  auto childNode = cm.getCallgraph()->getOrInsertNode("child1");
  cm.putEdge("main", "fileA.c", mainLineNumber, "child1", 10, 1, 0, 0);
  ASSERT_EQ(mainNode, cm.getCallgraph()->getOrInsertNode("main"));
  ASSERT_EQ(childNode, cm.getCallgraph()->getOrInsertNode("child1"));
  auto childNode2 = cm.getCallgraph()->getOrInsertNode("child2");
  cm.putEdge("main", "fileA.c", mainLineNumber, "child2", 12, 23, 0, 0);
  ASSERT_EQ(2, mainNode->getChildNodes().size());
  ASSERT_EQ(1, childNode->getParentNodes().size());
  ASSERT_EQ(1, childNode2->getParentNodes().size());
}

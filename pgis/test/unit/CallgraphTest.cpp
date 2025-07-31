/**
 * File: CallgraphTest.cpp
 * License: Part of the metacg project. Licensed under BSD 3 clause license. See LICENSE.txt file at
 * https://github.com/tudasc/metacg/LICENSE.txt
 */

#include "gtest/gtest.h"

// #include "LoggerUtil.h"

#include "../../../graph/include/Callgraph.h"
#include "../../../graph/include/LoggerUtil.h"

using namespace metacg;

class CallgraphTest : public ::testing::Test {
 protected:
  void SetUp() override { loggerutil::getLogger(); }
};

TEST_F(CallgraphTest, EmptyCG) {
  Callgraph c;
  ASSERT_TRUE(c.isEmpty());
  ASSERT_EQ(nullptr, c.getMain());
  ASSERT_EQ(0, c.size());
}

TEST_F(CallgraphTest, OnlyMainCG) {
  Callgraph c;
  ASSERT_TRUE(c.isEmpty());
  ASSERT_EQ(nullptr, c.getMain());
  auto n = c.insert("main");
  ASSERT_FALSE(c.isEmpty());
  ASSERT_EQ(n, c.getMain()->getId());
  ASSERT_EQ(n, c.getNode("main")->getId());
  ASSERT_EQ(n, (*(c.getNodes().begin())).second.get()->getId());
  ASSERT_EQ(true, c.hasNode("main"));
  ASSERT_EQ(1, c.size());
}

TEST_F(CallgraphTest, ClearEmptiesGraph) {
  Callgraph c;
  ASSERT_TRUE(c.isEmpty());
  auto n = std::make_unique<CgNode>("main");
  c.insert(std::move(n));
  ASSERT_FALSE(c.isEmpty());
  ASSERT_TRUE(c.hasNode("main"));  // sets lastSearched field
  c.clear();
  ASSERT_TRUE(c.isEmpty());
  ASSERT_EQ(nullptr, c.getMain());
}

TEST_F(CallgraphTest, TwoNodeConnectedCG) {
  Callgraph c;
  auto main = c.insert("main");
  auto child = c.insert("child");
  c.addEdge(main, child);
  ASSERT_EQ(2, c.size());
  ASSERT_EQ(true, c.hasNode("main"));
  ASSERT_EQ(true, c.hasNode("child"));
  ASSERT_EQ(main, c.getMain()->getId());
  auto foundMain = c.getMain();
  ASSERT_EQ(child, (*c.getCallees(foundMain).begin())->getId());
}

TEST_F(CallgraphTest, InsertTwiceTest) {
  Callgraph c;
  c.insert("node");
  ASSERT_EQ(1, c.size());
}

TEST_F(CallgraphTest, SearchNodes) {
  Callgraph c;
  auto node = c.insert("node1");
  auto node2 = c.insert("node2");
  c.addEdge(node, node2);
  ASSERT_EQ(nullptr, c.getMain());
  ASSERT_EQ(false, c.hasNode("main"));
  ASSERT_EQ(false, c.hasNode("nodeee"));
}

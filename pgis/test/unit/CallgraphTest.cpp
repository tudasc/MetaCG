/**
 * File: CallgraphTest.cpp
 * License: Part of the metacg project. Licensed under BSD 3 clause license. See LICENSE.txt file at
 * https://github.com/tudasc/metacg/LICENSE.txt
 */

#include "gtest/gtest.h"

#include "LoggerUtil.h"

#include "../../../graph/include/Callgraph.h"

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
  auto n = std::make_shared<CgNode>("main");
  c.insert(n);
  ASSERT_FALSE(c.isEmpty());
  ASSERT_EQ(n, c.getMain());
  ASSERT_EQ(n, c.getNode("main"));
  ASSERT_EQ(n, *(c.begin()));
  ASSERT_EQ(true, c.hasNode("main"));
  ASSERT_EQ(1, c.size());
}

TEST_F(CallgraphTest, ClearEmptiesGraph) {
  Callgraph c;
  ASSERT_TRUE(c.isEmpty());
  auto n = std::make_shared<CgNode>("main");
  c.insert(n);
  ASSERT_FALSE(c.isEmpty());
  ASSERT_TRUE(c.hasNode("main"));  // sets lastSearched field
  c.clear();
  ASSERT_TRUE(c.isEmpty());
  ASSERT_EQ(nullptr, c.getMain());
  ASSERT_EQ(nullptr, c.getLastSearchedNode());
}

TEST_F(CallgraphTest, TwoNodeConnectedCG) {
  Callgraph c;
  auto main = std::make_shared<CgNode>("main");
  auto child = std::make_shared<CgNode>("child");
  main->addChildNode(child);
  child->addParentNode(main);
  c.insert(main);
  c.insert(child);
  ASSERT_EQ(2, c.size());
  ASSERT_EQ(true, c.hasNode("main"));
  ASSERT_EQ(true, c.hasNode("child"));
  ASSERT_EQ(main, c.getMain());
  auto founMain = c.getMain();
  ASSERT_EQ(child, (*founMain->getChildNodes().begin()));
}

TEST_F(CallgraphTest, HasNodeGetLastSearchedTest) {
  Callgraph c;
  auto main = std::make_shared<CgNode>("child");
  c.insert(main);
  ASSERT_EQ(nullptr, c.getLastSearchedNode());
  c.hasNode("child");
  ASSERT_EQ(main, c.getLastSearchedNode());
}

TEST_F(CallgraphTest, InsertTwiceTest) {
  Callgraph c;
  auto node1 = std::make_shared<CgNode>("node");
  auto node2 = std::make_shared<CgNode>("node");
  c.insert(node1);
  c.insert(node2);
  ASSERT_EQ(1, c.size());
}

TEST_F(CallgraphTest, SearchNodes) {
  Callgraph c;
  auto node = std::make_shared<CgNode>("node1");
  c.insert(node);
  auto node2 = std::make_shared<CgNode>("node2");
  node2->addChildNode(node);
  node->addParentNode(node2);
  c.insert(node2);
  ASSERT_EQ(nullptr, c.getMain());
  ASSERT_EQ(false, c.hasNode("main"));
  ASSERT_EQ(false, c.hasNode("nodeee"));
  ASSERT_EQ(nullptr, c.getLastSearchedNode());
}

#include "gtest/gtest.h"

#include "CallgraphManager.h"

TEST(CallgraphManagerBasics, EmptyCG) {
  Config cfg;
  CallgraphManager cm(&cfg);
  ASSERT_EQ(0, cm.size());
  auto graph = cm.getCallgraph(&cm);
  ASSERT_EQ(false, graph.hasNode("main"));
  ASSERT_EQ(nullptr, graph.findMain());
  ASSERT_EQ(0, graph.size());
}

TEST(CallgraphManagerBasics, OneNodeCG) {
  Config cfg;
  CallgraphManager cm(&cfg);
  cm.findOrCreateNode("main");
  cm.setNodeComesFromCube("main");
  auto nPtr = cm.findOrCreateNode("main");
  auto graph = cm.getCallgraph(&cm);
  ASSERT_NE(nullptr, graph.findMain());
  ASSERT_EQ(nPtr, graph.findMain());
  ASSERT_EQ(true, nPtr->comesFromCube());
}

TEST(CallgraphManagerBasics, TwoNodeCG) {
  Config cfg;
  CallgraphManager cm(&cfg);
  int mainLineNumber = 1;
  auto mainNode = cm.findOrCreateNode("main");
  auto childNode = cm.findOrCreateNode("child1");
  cm.putEdge("main", "fileA.c", mainLineNumber, "child1", 10, 1, 0, 0);
  ASSERT_EQ(mainNode, cm.findOrCreateNode("main"));
  ASSERT_EQ(childNode, cm.findOrCreateNode("child1"));
  auto graph = cm.getCallgraph(&cm);
  ASSERT_EQ(mainNode, graph.findMain());
  ASSERT_EQ(childNode, graph.findNode("child1"));
  auto mainNode_f = graph.findMain();
  ASSERT_EQ(mainLineNumber, mainNode->getLineNumber());
}

TEST(CallgraphManagerBasics, ThreeNodeCG) {
  Config cfg;
  CallgraphManager cm(&cfg);
  int mainLineNumber = 1;
  auto mainNode = cm.findOrCreateNode("main");
  auto childNode = cm.findOrCreateNode("child1");
  cm.putEdge("main", "fileA.c", mainLineNumber, "child1", 10, 1, 0, 0);
  ASSERT_EQ(mainNode, cm.findOrCreateNode("main"));
  ASSERT_EQ(childNode, cm.findOrCreateNode("child1"));
  auto childNode2 = cm.findOrCreateNode("child2");
  cm.putEdge("main", "fileA.c", mainLineNumber, "child2", 12, 23, 0, 0);
  ASSERT_EQ(2, mainNode->getChildNodes().size());
  ASSERT_EQ(1, childNode->getParentNodes().size());
  ASSERT_EQ(1, childNode2->getParentNodes().size());
}

/**
 * File: MCGManagerTest.cpp
 * License: Part of the metacg project. Licensed under BSD 3 clause license. See LICENSE.txt file at
 * https://github.com/tudasc/metacg/LICENSE.txt
 */

#include "gtest/gtest.h"

#include "MCGManager.h"
#include "metadata/EntryFunctionMD.h"
#include "metadata/MetaData.h"
#include "metadata/OverrideMD.h"

using json = nlohmann::json;

// TODO: These tests mostly test the CG itself and not so much the manager anymore.
//       If/when we decide to remove the manager, these tests should be renamed accordingly.

class MCGManagerTest : public ::testing::Test {
 protected:
  void SetUp() override {
    auto& mcgm = metacg::graph::MCGManager::get();
    mcgm.resetManager();
    mcgm.addToManagedGraphs("emptyGraph", std::make_unique<metacg::Callgraph>());
  }
};

TEST_F(MCGManagerTest, EmptyCG) {
  auto& mcgm = metacg::graph::MCGManager::get();
  ASSERT_EQ(0, mcgm.size());

  auto graph = mcgm.getCallgraph();

  ASSERT_TRUE(graph->isEmpty());
  ASSERT_EQ(false, graph->hasNode("main"));
  ASSERT_EQ(nullptr, graph->getMain());
  ASSERT_EQ(0, graph->size());
}

TEST_F(MCGManagerTest, GetOrCreateCGActive) {
  auto& mcgm = metacg::graph::MCGManager::get();
  auto* cg = mcgm.getOrCreateCallgraph("newCG", true);
  ASSERT_NE(nullptr, cg);
  ASSERT_TRUE(mcgm.assertActive("newCG"));
}

TEST_F(MCGManagerTest, GetOrCreateCGExistingActive) {
  auto& mcgm = metacg::graph::MCGManager::get();
  mcgm.addToManagedGraphs("newCG", std::make_unique<metacg::Callgraph>(), false);
  mcgm.addToManagedGraphs("newCG2", std::make_unique<metacg::Callgraph>(), false);
  ASSERT_FALSE(mcgm.assertActive("newCG"));
  ASSERT_FALSE(mcgm.assertActive("newCG2"));
  mcgm.addToManagedGraphs("newCG", std::make_unique<metacg::Callgraph>(), true);
  ASSERT_TRUE(mcgm.assertActive("newCG"));
  ASSERT_FALSE(mcgm.assertActive("newCG2"));
}

TEST_F(MCGManagerTest, OneNodeCG) {
  auto& mcgm = metacg::graph::MCGManager::get();
  mcgm.getCallgraph()->getOrInsertNode("main");
  auto& n = mcgm.getCallgraph()->getOrInsertNode("main");
  auto graph = mcgm.getCallgraph();
  ASSERT_FALSE(graph->isEmpty());
  ASSERT_NE(nullptr, graph->getMain());
  ASSERT_EQ(&n, graph->getMain());
}

TEST_F(MCGManagerTest, TwoNodeCG) {
  auto& mcgm = metacg::graph::MCGManager::get();
  auto& mainNode = mcgm.getCallgraph()->getOrInsertNode("main");
  auto& childNode = mcgm.getCallgraph()->getOrInsertNode("child1");
  mcgm.getCallgraph()->addEdge("main", "child1");
  ASSERT_EQ(&mainNode, &mcgm.getCallgraph()->getOrInsertNode("main"));
  ASSERT_EQ(&childNode, &mcgm.getCallgraph()->getOrInsertNode("child1"));
  auto graph = mcgm.getCallgraph();
  ASSERT_EQ(&mainNode, graph->getMain());
  ASSERT_EQ(&childNode, graph->getFirstNode("child1"));
}

TEST_F(MCGManagerTest, ThreeNodeCG) {
  auto& mcgm = metacg::graph::MCGManager::get();
  auto& mainNode = mcgm.getCallgraph()->getOrInsertNode("main");
  auto& childNode = mcgm.getCallgraph()->getOrInsertNode("child1");
  mcgm.getCallgraph()->addEdge("main", "child1");
  ASSERT_EQ(&mainNode, &mcgm.getCallgraph()->getOrInsertNode("main"));
  ASSERT_EQ(&childNode, &mcgm.getCallgraph()->getOrInsertNode("child1"));
  auto& childNode2 = mcgm.getCallgraph()->getOrInsertNode("child2");
  mcgm.getCallgraph()->addEdge("main", "child2");
  ASSERT_EQ(2, mcgm.getCallgraph()->getCallees(mainNode.getId()).size());
  ASSERT_EQ(1, mcgm.getCallgraph()->getCallers(childNode.getId()).size());
  ASSERT_EQ(1, mcgm.getCallgraph()->getCallers(childNode2.getId()).size());
}

TEST_F(MCGManagerTest, TwoNodeOneEdgeCG) {
  auto& mcgm = metacg::graph::MCGManager::get();
  auto cg = mcgm.getCallgraph();
  cg->addEdge(cg->getOrInsertNode("main"), cg->getOrInsertNode("LC1"));

  ASSERT_TRUE(cg->getMain() != nullptr);
  auto mainNode = cg->getMain();
  ASSERT_TRUE(mcgm.getCallgraph()->getCallees(mainNode->getId()).size() == 1);
}

TEST_F(MCGManagerTest, GetSingleNode) {
  auto& mcgm = metacg::graph::MCGManager::get();
  auto& cg = *mcgm.getCallgraph();
  cg.insert("main");
  ASSERT_EQ(cg.getSingleNode("main").getFunctionName(), "main");
  cg.insert("main");
  ASSERT_DEBUG_DEATH(cg.getSingleNode("main"), "There must be exactly one node with this name");
}

TEST_F(MCGManagerTest, EraseNodeNoEdge) {
  auto& mcgm = metacg::graph::MCGManager::get();
  auto& cg = *mcgm.getCallgraph();
  auto& mainNode = cg.getOrInsertNode("main");
  int mainNodeId = mainNode.getId();
  ASSERT_TRUE(cg.erase(mainNode.getId()));
  ASSERT_FALSE(cg.hasNode("main"));
  ASSERT_FALSE(cg.hasNode(mainNodeId));
  ASSERT_EQ(cg.size(), 1);
  ASSERT_EQ(cg.getNodeCount(), 0);
  ASSERT_TRUE(cg.isEmpty());
}

TEST_F(MCGManagerTest, EraseNodeWithEdge) {
  auto& mcgm = metacg::graph::MCGManager::get();
  auto& cg = *mcgm.getCallgraph();
  auto& mainNode = cg.getOrInsertNode("main");
  auto& childNode = cg.getOrInsertNode("child1");
  int mainNodeId = mainNode.getId();
  int childNodeId = childNode.getId();
  ASSERT_TRUE(cg.addEdge(mainNodeId, childNodeId));
  ASSERT_TRUE(cg.erase(mainNode.getId()));
  ASSERT_FALSE(cg.hasNode("main"));
  ASSERT_FALSE(cg.hasNode(mainNodeId));
  ASSERT_EQ(cg.size(), 2);
  ASSERT_EQ(cg.getNodeCount(), 1);
  ASSERT_FALSE(cg.isEmpty());
  ASSERT_FALSE(cg.existsEdge(mainNodeId, childNodeId));
}

TEST_F(MCGManagerTest, RemoveEdge) {
  auto& mcgm = metacg::graph::MCGManager::get();
  auto& cg = *mcgm.getCallgraph();
  auto& mainNode = cg.getOrInsertNode("main");
  auto& childNode = cg.getOrInsertNode("child1");
  int mainNodeId = mainNode.getId();
  int childNodeId = childNode.getId();
  ASSERT_TRUE(cg.addEdge(mainNodeId, childNodeId));
  ASSERT_TRUE(cg.removeEdge(mainNodeId, childNodeId));
  ASSERT_FALSE(cg.existsEdge(mainNodeId, childNodeId));
}

TEST_F(MCGManagerTest, HasNode) {
  auto& mcgm = metacg::graph::MCGManager::get();
  auto& cg = *mcgm.getCallgraph();
  auto& mainNode = cg.getOrInsertNode("main");

  auto& cg2 = *mcgm.getOrCreateCallgraph("secondCG");
  auto& otherMain = cg2.getOrInsertNode("main");

  ASSERT_TRUE(cg.hasNode(mainNode));
  ASSERT_TRUE(cg.hasNode("main"));
  ASSERT_TRUE(cg.hasNode(mainNode.getId()));
  ASSERT_FALSE(cg.hasNode(otherMain));

  ASSERT_TRUE(cg2.hasNode(otherMain));
  ASSERT_TRUE(cg2.hasNode("main"));
  ASSERT_TRUE(cg2.hasNode(otherMain.getId()));
  ASSERT_FALSE(cg2.hasNode(mainNode));
}

TEST_F(MCGManagerTest, EdgeToNodeFromOtherCG) {
  auto& mcgm = metacg::graph::MCGManager::get();
  auto& cg = *mcgm.getCallgraph();
  auto& mainNode = cg.getOrInsertNode("main");
  auto& childNode = cg.getOrInsertNode("child1");

  auto& cg2 = *mcgm.getOrCreateCallgraph("secondCG");
  auto& otherMain = cg2.getOrInsertNode("main");

  // Try to add edge between nodes of different call graphs
  cg.addEdge(otherMain, childNode);

  ASSERT_FALSE(cg.existsEdge(otherMain, childNode));
  ASSERT_FALSE(cg.existsEdge(mainNode, childNode));
}

TEST_F(MCGManagerTest, AddMDtoEdge) {
  auto& mcgm = metacg::graph::MCGManager::get();
  auto cg = mcgm.getCallgraph();
  cg->addEdge(cg->getOrInsertNode("main"), cg->getOrInsertNode("LC1"));
  ASSERT_TRUE(cg->getMain() != nullptr);
  ASSERT_TRUE(cg->getFirstNode("LC1"));

  cg->addEdgeMetaData(*cg->getMain(), *cg->getFirstNode("LC1"), std::make_unique<metacg::OverrideMD>());
  cg->hasEdgeMetaData<metacg::OverrideMD>(*cg->getMain(), *cg->getFirstNode("LC1"));
}

TEST_F(MCGManagerTest, AddMDtoMissingEdge) {
  auto& mcgm = metacg::graph::MCGManager::get();
  auto cg = mcgm.getCallgraph();
  auto& main = cg->getOrInsertNode("main");
  auto& foo = cg->getOrInsertNode("foo");
  ASSERT_FALSE(cg->addEdgeMetaData(main, foo, std::make_unique<metacg::OverrideMD>()));
}

TEST_F(MCGManagerTest, ComplexCG) {
  // Call-order: top to bottom or arrow if given
  /*
   *              main
   *            /     \
   *          1        2<--|
   *         / \       |\__|
   *        3-->4      5
   *              \    |
   *               \-->6
   *
   */

  auto& mcgm = metacg::graph::MCGManager::get();
  auto& mainNode = mcgm.getCallgraph()->getOrInsertNode("main");
  auto& childNode1 = mcgm.getCallgraph()->getOrInsertNode("child1");
  auto& childNode2 = mcgm.getCallgraph()->getOrInsertNode("child2");
  auto& childNode3 = mcgm.getCallgraph()->getOrInsertNode("child3");
  auto& childNode4 = mcgm.getCallgraph()->getOrInsertNode("child4");
  auto& childNode5 = mcgm.getCallgraph()->getOrInsertNode("child5");
  auto& childNode6 = mcgm.getCallgraph()->getOrInsertNode("child6");
  mcgm.getCallgraph()->addEdge("main", "child1");
  mcgm.getCallgraph()->addEdge("main", "child2");
  mcgm.getCallgraph()->addEdge("child1", "child3");
  mcgm.getCallgraph()->addEdge("child1", "child4");
  mcgm.getCallgraph()->addEdge("child3", "child4");
  mcgm.getCallgraph()->addEdge("child4", "child6");
  mcgm.getCallgraph()->addEdge("child2", "child5");
  mcgm.getCallgraph()->addEdge("child2", "child2");
  mcgm.getCallgraph()->addEdge("child5", "child6");

  // Main has 2 children: child 1 and 2
  ASSERT_EQ(mcgm.getCallgraph()->getCallees(mainNode.getId()).size(), 2);
  for (const auto& elem : mcgm.getCallgraph()->getCallees(mainNode.getId())) {
    ASSERT_TRUE(elem->getFunctionName() == "child1" || elem->getFunctionName() == "child2");
  }
  // Child 1 has 2 children: child 3 and 4
  ASSERT_EQ(mcgm.getCallgraph()->getCallees(childNode1.getId()).size(), 2);
  for (const auto& elem : mcgm.getCallgraph()->getCallees(childNode1.getId())) {
    ASSERT_TRUE(elem->getFunctionName() == "child3" || elem->getFunctionName() == "child4");
  }
  // Child 2 has 2 child: child2 and child 5
  ASSERT_EQ(mcgm.getCallgraph()->getCallees(childNode2.getId()).size(), 2);
  for (const auto& elem : mcgm.getCallgraph()->getCallees(childNode2.getId())) {
    ASSERT_TRUE(elem->getFunctionName() == "child2" || elem->getFunctionName() == "child5");
  }
  // Child 3 has 1 child: child 4
  ASSERT_EQ(mcgm.getCallgraph()->getCallees(childNode3.getId()).size(), 1);
  ASSERT_TRUE((*mcgm.getCallgraph()->getCallees(childNode3.getId()).begin())->getFunctionName() == "child4");
  // Child 4 has 1 child: child 6
  ASSERT_EQ(mcgm.getCallgraph()->getCallees(childNode4.getId()).size(), 1);
  ASSERT_TRUE((*mcgm.getCallgraph()->getCallees(childNode4.getId()).begin())->getFunctionName() == "child6");
  // Child 5 has 1 child: child 6
  ASSERT_EQ(mcgm.getCallgraph()->getCallees(childNode5.getId()).size(), 1);
  ASSERT_TRUE((*mcgm.getCallgraph()->getCallees(childNode5.getId()).begin())->getFunctionName() == "child6");
  // Child 6 has 0 children:
  ASSERT_EQ(mcgm.getCallgraph()->getCallees(childNode6.getId()).size(), 0);

  // Main has 0 parents:
  ASSERT_EQ(mcgm.getCallgraph()->getCallers(mainNode.getId()).size(), 0);
  // Child 1 has 1 parent: main
  ASSERT_EQ(mcgm.getCallgraph()->getCallers(childNode1.getId()).size(), 1);
  ASSERT_TRUE((*mcgm.getCallgraph()->getCallers(childNode1.getId()).begin())->getFunctionName() == "main");
  // Child 2 has 2 parents: main and child2
  ASSERT_EQ(mcgm.getCallgraph()->getCallers(childNode2.getId()).size(), 2);
  for (const auto& elem : mcgm.getCallgraph()->getCallers(childNode2.getId())) {
    ASSERT_TRUE(elem->getFunctionName() == "main" || elem->getFunctionName() == "child2");
  }
  // Child 3 has 1 parent: child 1
  ASSERT_EQ(mcgm.getCallgraph()->getCallers(childNode3.getId()).size(), 1);
  ASSERT_TRUE((*mcgm.getCallgraph()->getCallers(childNode3.getId()).begin())->getFunctionName() == "child1");
  // Child 4 has 2 parents: child 1 and child 3
  ASSERT_EQ(mcgm.getCallgraph()->getCallers(childNode4.getId()).size(), 2);
  for (const auto& elem : mcgm.getCallgraph()->getCallers(childNode4.getId())) {
    ASSERT_TRUE(elem->getFunctionName() == "child1" || elem->getFunctionName() == "child3");
  }
  // Child 5 has 1 parent: child 2
  ASSERT_EQ(mcgm.getCallgraph()->getCallers(childNode5.getId()).size(), 1);
  ASSERT_TRUE((*mcgm.getCallgraph()->getCallers(childNode5.getId()).begin())->getFunctionName() == "child2");
  // Child 6 has 2 parents: child 4 and child 5
  ASSERT_EQ(mcgm.getCallgraph()->getCallers(childNode6.getId()).size(), 2);
  for (const auto& elem : mcgm.getCallgraph()->getCallers(childNode6.getId())) {
    ASSERT_TRUE(elem->getFunctionName() == "child4" || elem->getFunctionName() == "child5");
  }
}

// Callgraph merge functionality from here on out. Exclusively structure checks

TEST_F(MCGManagerTest, EmptyCallgraphMerge) {
  auto& mcgm = metacg::graph::MCGManager::get();
  mcgm.addToManagedGraphs("newCG", std::make_unique<metacg::Callgraph>(), true);
  mcgm.addToManagedGraphs("newCG2", std::make_unique<metacg::Callgraph>(), false);
  mcgm.mergeIntoActiveGraph(metacg::MergeByName());
  ASSERT_TRUE(mcgm.getCallgraph()->isEmpty());
}

TEST_F(MCGManagerTest, FullIntoEmptyCallgraphMerge) {
  auto& mcgm = metacg::graph::MCGManager::get();
  mcgm.addToManagedGraphs("newCG", std::make_unique<metacg::Callgraph>(), true);
  mcgm.addToManagedGraphs("newCG2", std::make_unique<metacg::Callgraph>(), false);

  mcgm.getCallgraph("newCG2", false)->insert("main");
  mcgm.getCallgraph("newCG2", false)->insert("child1");
  mcgm.getCallgraph("newCG2", false)->getFirstNode("main")->setHasBody(true);
  mcgm.getCallgraph("newCG2", false)->getFirstNode("child1")->setHasBody(true);
  mcgm.getCallgraph("newCG2", false)->addEdge("main", "child1");

  mcgm.mergeIntoActiveGraph(metacg::MergeByName());

  ASSERT_TRUE(mcgm.getCallgraph()->hasNode("main"));
  ASSERT_TRUE(mcgm.getCallgraph()->hasNode("child1"));
  ASSERT_TRUE(mcgm.getCallgraph()->getFirstNode("main")->getHasBody());
  ASSERT_TRUE(mcgm.getCallgraph()->getFirstNode("child1")->getHasBody());
  ASSERT_TRUE(mcgm.getCallgraph()->existsAnyEdge("main", "child1"));
}

TEST_F(MCGManagerTest, FullIntoFullCallgraphMerge) {
  auto& mcgm = metacg::graph::MCGManager::get();
  mcgm.addToManagedGraphs("newCG", std::make_unique<metacg::Callgraph>(), true);
  mcgm.addToManagedGraphs("newCG2", std::make_unique<metacg::Callgraph>(), false);

  mcgm.getCallgraph()->insert("child2");
  mcgm.getCallgraph()->insert("child1");
  mcgm.getCallgraph()->getFirstNode("child2")->setHasBody(true);
  mcgm.getCallgraph()->getFirstNode("child1")->setHasBody(true);
  mcgm.getCallgraph()->addEdge("child1", "child2");

  mcgm.getCallgraph("newCG2", false)->insert("main");
  mcgm.getCallgraph("newCG2", false)->insert("child1");
  mcgm.getCallgraph("newCG2", false)->getFirstNode("main")->setHasBody(true);
  mcgm.getCallgraph("newCG2", false)->getFirstNode("child1")->setHasBody(true);
  mcgm.getCallgraph("newCG2", false)->addEdge("main", "child1");

  mcgm.mergeIntoActiveGraph(metacg::MergeByName());

  ASSERT_TRUE(mcgm.getCallgraph()->hasNode("main"));
  ASSERT_TRUE(mcgm.getCallgraph()->hasNode("child1"));
  ASSERT_TRUE(mcgm.getCallgraph()->hasNode("child2"));
  ASSERT_TRUE(mcgm.getCallgraph()->getFirstNode("main")->getHasBody());
  ASSERT_TRUE(mcgm.getCallgraph()->getFirstNode("child1")->getHasBody());
  ASSERT_TRUE(mcgm.getCallgraph()->getFirstNode("child2")->getHasBody());
  ASSERT_TRUE(mcgm.getCallgraph()->existsAnyEdge("main", "child1"));
  ASSERT_TRUE(mcgm.getCallgraph()->existsAnyEdge("child1", "child2"));
}

TEST_F(MCGManagerTest, GetMainTest) {
  auto& mcgm = metacg::graph::MCGManager::get();
  mcgm.addToManagedGraphs("newCG", std::make_unique<metacg::Callgraph>(), true);

  auto cg = mcgm.getCallgraph();
  auto& main = cg->getOrInsertNode("main");
  auto* detectedMain = cg->getMain();
  ASSERT_EQ(detectedMain->getId(), main.getId());

  auto& realMain = cg->getOrInsertNode("thisIsTheRealMain");
  cg->getOrCreate<metacg::EntryFunctionMD>(realMain);
  detectedMain = cg->getMain(true);
  ASSERT_EQ(detectedMain->getId(), realMain.getId());
}
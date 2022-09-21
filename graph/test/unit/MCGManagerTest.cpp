/**
 * File: MCGManagerTest.cpp
 * License: Part of the metacg project. Licensed under BSD 3 clause license. See LICENSE.txt file at
 * https://github.com/tudasc/metacg/LICENSE.txt
 */

#include "gtest/gtest.h"

#include "LoggerUtil.h"

#include "MCGManager.h"
#include "MetaData.h"

using json = nlohmann::json;

/**
 * This is to test, if it can actually work as imagined
 */
struct TestHandler : public metacg::io::retriever::MetaDataHandler {
  int i{0};
  const std::string toolName() const override { return "TestMetaHandler"; }
  void read([[maybe_unused]] const json &j, const std::string &functionName) override { i++; }
  bool handles(const CgNodePtr n) const override { return false; }
  json value(const CgNodePtr n) const override {
    json j;
    j = i;
    return j;
  }
};

class MCGManagerTest : public ::testing::Test {
 protected:
  void SetUp() override {
    metacg::loggerutil::getLogger();
    auto &mcgm = metacg::graph::MCGManager::get();
    mcgm.resetManager();
    mcgm.addToManagedGraphs("emptyGraph",std::make_unique<metacg::Callgraph>());
  }
};

TEST_F(MCGManagerTest, EmptyCG) {

  auto &mcgm = metacg::graph::MCGManager::get();
  ASSERT_EQ(0, mcgm.size());

  auto graph = *mcgm.getCallgraph();

  ASSERT_TRUE(graph.isEmpty());
  ASSERT_EQ(false, graph.hasNode("main"));
  ASSERT_EQ(nullptr, graph.getMain());
  ASSERT_EQ(0, graph.size());
}

TEST_F(MCGManagerTest, OneNodeCG) {
  auto &mcgm = metacg::graph::MCGManager::get();
  mcgm.findOrCreateNode("main");
  auto nPtr = mcgm.findOrCreateNode("main");
  auto graph = *mcgm.getCallgraph();
  ASSERT_FALSE(graph.isEmpty());
  ASSERT_NE(nullptr, graph.getMain());
  ASSERT_EQ(nPtr, graph.getMain());
}

TEST_F(MCGManagerTest, TwoNodeCG) {
  auto &mcgm = metacg::graph::MCGManager::get();
  auto mainNode = mcgm.findOrCreateNode("main");
  auto childNode = mcgm.findOrCreateNode("child1");
  mcgm.addEdge("main", "child1");
  ASSERT_EQ(mainNode, mcgm.findOrCreateNode("main"));
  ASSERT_EQ(childNode, mcgm.findOrCreateNode("child1"));
  auto graph = *mcgm.getCallgraph();
  ASSERT_EQ(mainNode, graph.getMain());
  ASSERT_EQ(childNode, graph.getNode("child1"));
}

TEST_F(MCGManagerTest, ThreeNodeCG) {
  auto &mcgm = metacg::graph::MCGManager::get();
  auto mainNode = mcgm.findOrCreateNode("main");
  auto childNode = mcgm.findOrCreateNode("child1");
  mcgm.addEdge("main", "child1");
  ASSERT_EQ(mainNode, mcgm.findOrCreateNode("main"));
  ASSERT_EQ(childNode, mcgm.findOrCreateNode("child1"));
  auto childNode2 = mcgm.findOrCreateNode("child2");
  mcgm.addEdge("main", "child2");
  ASSERT_EQ(2, mainNode->getChildNodes().size());
  ASSERT_EQ(1, childNode->getParentNodes().size());
  ASSERT_EQ(1, childNode2->getParentNodes().size());
}

TEST_F(MCGManagerTest, OneMetaDataAttached) {
  auto &mcgm = metacg::graph::MCGManager::get();
  mcgm.addMetaHandler<TestHandler>();
  const auto &handlers = mcgm.getMetaHandlers();
  ASSERT_EQ(handlers.size(), 1);
}

TEST_F(MCGManagerTest, TwoNodeOneEdgeCG) {
  auto &mcgm = metacg::graph::MCGManager::get();
  auto cg = mcgm.getCallgraph();
  cg->addEdge("main", "LC1");
  ASSERT_TRUE(cg->getMain() != nullptr);
  auto mainNode = cg->getMain();
  ASSERT_TRUE(mainNode->getChildNodes().size() == 1);
}
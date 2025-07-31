/**
 * File: IPCGEstimatorPhaseTest.cpp
 * License: Part of the metacg project. Licensed under BSD 3 clause license. See LICENSE.txt file at
 * https://github.com/tudasc/metacg/LICENSE.txt
 */

#include "IPCGEstimatorPhase.h"
#include "CgHelper.h"
#include "LoggerUtil.h"
#include "MCGManager.h"
#include "PiraMCGProcessor.h"

#include "gtest/gtest.h"

class IPCGEstimatorPhaseBasic : public ::testing::Test {
 protected:
  void SetUp() override {
    metacg::loggerutil::getLogger();
    auto& mcgm = metacg::graph::MCGManager::get();
    mcgm.resetManager();
    mcgm.addToManagedGraphs("emptyGraph", std::make_unique<metacg::Callgraph>());
  }

  static void attachAllMetaDataToGraph(metacg::Callgraph* cg) {
    metacg::pgis::attachMetaDataToGraph<pira::BaseProfileData>(cg);
    metacg::pgis::attachMetaDataToGraph<pira::PiraOneData>(cg);
    metacg::pgis::attachMetaDataToGraph<pira::PiraTwoData>(cg);
  }
};

TEST_F(IPCGEstimatorPhaseBasic, EmptyCG) {
  // XXX this is not ideal, but required for the death assert
  LOGGERUTIL_ENABLE_ERRORS_LOCAL
  Config cfg;
  auto& mcgm = metacg::graph::MCGManager::get();
  auto& cm = metacg::pgis::PiraMCGProcessor::get();
  mcgm.addToManagedGraphs("emptyGraph", std::make_unique<metacg::Callgraph>());
  attachAllMetaDataToGraph(mcgm.getCallgraph());
  cm.setCG(mcgm.getCallgraph());
  cm.setConfig(&cfg);
  cm.setNoOutput();
  StatementCountEstimatorPhase sce(10, mcgm.getCallgraph());
  cm.registerEstimatorPhase(&sce);
  ASSERT_DEATH(cm.applyRegisteredPhases(), "Running the processor on empty graph. Need to construct graph.");
  cm.removeAllEstimatorPhases();
}

TEST_F(IPCGEstimatorPhaseBasic, OneNodeCG) {
  Config cfg;
  auto& cm = metacg::pgis::PiraMCGProcessor::get();
  auto& mcgm = metacg::graph::MCGManager::get();
  mcgm.resetManager();
  cm.setConfig(&cfg);
  cm.setNoOutput();
  mcgm.addToManagedGraphs("graph", std::make_unique<metacg::Callgraph>());
  auto mainNode = &mcgm.getCallgraph()->getOrInsertNode("main");
  ASSERT_NE(mainNode, nullptr);
  attachAllMetaDataToGraph(mcgm.getCallgraph());
  cm.setCG(mcgm.getCallgraph());
  ASSERT_FALSE(mcgm.getCallgraph()->isEmpty());
  StatementCountEstimatorPhase sce(10, mcgm.getCallgraph());
  sce.setNoReport();
  cm.registerEstimatorPhase(&sce);
  cm.applyRegisteredPhases();
  auto graph = mcgm.getCallgraph();
  ASSERT_EQ(mainNode, graph->getMain());
  ASSERT_EQ(0, sce.getNumStatements(mainNode));
  cm.removeAllEstimatorPhases();
}

TEST_F(IPCGEstimatorPhaseBasic, TwoNodeCG) {
  Config cfg;
  auto& cm = metacg::pgis::PiraMCGProcessor::get();
  auto& mcgm = metacg::graph::MCGManager::get();
  cm.setConfig(&cfg);
  cm.setNoOutput();
  mcgm.addToManagedGraphs("graph", std::make_unique<metacg::Callgraph>());
  auto mainNode = &mcgm.getCallgraph()->getOrInsertNode("main");
  auto childNode = &mcgm.getCallgraph()->getOrInsertNode("child1");
  mcgm.getCallgraph()->addEdge(*mainNode, *childNode);
  attachAllMetaDataToGraph(mcgm.getCallgraph());
  cm.setCG(mcgm.getCallgraph());
  StatementCountEstimatorPhase sce(10, mcgm.getCallgraph());
  sce.setNoReport();
  cm.registerEstimatorPhase(&sce);
  cm.applyRegisteredPhases();
  auto graph = cm.getCallgraph(&cm);
  ASSERT_EQ(mainNode, graph->getMain());
  ASSERT_EQ(0, sce.getNumStatements(mainNode));
  cm.removeAllEstimatorPhases();
}

TEST_F(IPCGEstimatorPhaseBasic, OneNodeCGwStmt) {
  Config cfg;
  auto& cm = metacg::pgis::PiraMCGProcessor::get();
  auto& mcgm = metacg::graph::MCGManager::get();
  cm.setConfig(&cfg);
  cm.setNoOutput();
  mcgm.addToManagedGraphs("graph", std::make_unique<metacg::Callgraph>());
  auto mainNode = &mcgm.getCallgraph()->getOrInsertNode("main");
  attachAllMetaDataToGraph(mcgm.getCallgraph());
  const auto data = mainNode->get<pira::PiraOneData>();
  if (data) {
    data->setNumberOfStatements(12);
    data->setHasBody();
  } else {
    assert(false && "Nodes should have PIRA I data attached.");
  }
  cm.setCG(mcgm.getCallgraph());
  StatementCountEstimatorPhase sce(10, mcgm.getCallgraph());
  sce.setNoReport();
  cm.registerEstimatorPhase(&sce);
  cm.applyRegisteredPhases();
  auto graph = cm.getCallgraph(&cm);
  ASSERT_EQ(mainNode, graph->getMain());
  ASSERT_EQ(12, sce.getNumStatements(mainNode));
  cm.removeAllEstimatorPhases();
}

TEST_F(IPCGEstimatorPhaseBasic, TwoNodeCGwStmt) {
  Config cfg;
  auto& cm = metacg::pgis::PiraMCGProcessor::get();
  auto& mcgm = metacg::graph::MCGManager::get();
  cm.setConfig(&cfg);
  cm.setNoOutput();

  mcgm.addToManagedGraphs("graph", std::make_unique<metacg::Callgraph>());
  auto mainNode = &mcgm.getCallgraph()->getOrInsertNode("main");
  attachAllMetaDataToGraph(mcgm.getCallgraph());
  pira::setPiraOneData(mainNode, 12, true);

  auto childNode = &mcgm.getCallgraph()->getOrInsertNode("child1");
  attachAllMetaDataToGraph(mcgm.getCallgraph());
  pira::setPiraOneData(childNode, 7, true);

  mcgm.getCallgraph()->addEdge(*mainNode, *childNode);
  cm.setCG(mcgm.getCallgraph());

  StatementCountEstimatorPhase sce(10, mcgm.getCallgraph());
  sce.setNoReport();
  cm.registerEstimatorPhase(&sce);
  cm.applyRegisteredPhases();
  auto graph = cm.getCallgraph(&cm);
  ASSERT_EQ(mainNode, graph->getMain());
  ASSERT_EQ(19, sce.getNumStatements(mainNode));
  ASSERT_EQ(7, sce.getNumStatements(childNode));
  cm.removeAllEstimatorPhases();
}

TEST_F(IPCGEstimatorPhaseBasic, ThreeNodeCGwStmt) {
  Config cfg;
  auto& cm = metacg::pgis::PiraMCGProcessor::get();
  auto& mcgm = metacg::graph::MCGManager::get();

  cm.setConfig(&cfg);
  cm.setNoOutput();

  mcgm.addToManagedGraphs("graph", std::make_unique<metacg::Callgraph>());
  auto mainNode = &mcgm.getCallgraph()->getOrInsertNode("main");
  attachAllMetaDataToGraph(mcgm.getCallgraph());
  pira::setPiraOneData(mainNode, 12, true);

  auto childNode = &mcgm.getCallgraph()->getOrInsertNode("child1");
  attachAllMetaDataToGraph(mcgm.getCallgraph());
  pira::setPiraOneData(childNode, 7, true);
  mcgm.getCallgraph()->addEdge(*mainNode, *childNode);

  auto childNode2 = &mcgm.getCallgraph()->getOrInsertNode("child2");
  attachAllMetaDataToGraph(mcgm.getCallgraph());
  pira::setPiraOneData(childNode2, 42, true);
  mcgm.getCallgraph()->addEdge(*mainNode, *childNode2);

  cm.setCG(mcgm.getCallgraph());

  StatementCountEstimatorPhase sce(10, mcgm.getCallgraph());
  sce.setNoReport();
  cm.registerEstimatorPhase(&sce);
  cm.applyRegisteredPhases();
  auto graph = cm.getCallgraph(&cm);
  ASSERT_EQ(mainNode, graph->getMain());
  ASSERT_EQ(61, sce.getNumStatements(mainNode));
  ASSERT_EQ(7, sce.getNumStatements(childNode));
  ASSERT_EQ(42, sce.getNumStatements(childNode2));
  cm.removeAllEstimatorPhases();
}

TEST_F(IPCGEstimatorPhaseBasic, ThreeNodeCycleCGwStmt) {
  Config cfg;
  auto& cm = metacg::pgis::PiraMCGProcessor::get();
  auto& mcgm = metacg::graph::MCGManager::get();

  cm.setConfig(&cfg);
  cm.setNoOutput();
  mcgm.addToManagedGraphs("graph", std::make_unique<metacg::Callgraph>());
  auto mainNode = &mcgm.getCallgraph()->getOrInsertNode("main");
  attachAllMetaDataToGraph(mcgm.getCallgraph());
  pira::setPiraOneData(mainNode, 12, true);

  auto childNode = &mcgm.getCallgraph()->getOrInsertNode("child1");
  attachAllMetaDataToGraph(mcgm.getCallgraph());
  pira::setPiraOneData(childNode, 7, true);
  mcgm.getCallgraph()->addEdge(*mainNode, *childNode);

  auto childNode2 = &mcgm.getCallgraph()->getOrInsertNode("child2");
  attachAllMetaDataToGraph(mcgm.getCallgraph());
  pira::setPiraOneData(childNode2, 42, true);
  mcgm.getCallgraph()->addEdge(*childNode, *childNode2);
  mcgm.getCallgraph()->addEdge(*childNode2, *childNode);

  cm.setCG(mcgm.getCallgraph());

  StatementCountEstimatorPhase sce(10, mcgm.getCallgraph());
  sce.setNoReport();
  cm.registerEstimatorPhase(&sce);
  cm.applyRegisteredPhases();
  auto graph = cm.getCallgraph(&cm);
  ASSERT_EQ(mainNode, graph->getMain());
  ASSERT_EQ(61, sce.getNumStatements(mainNode));
  ASSERT_EQ(49, sce.getNumStatements(childNode));
  ASSERT_EQ(49, sce.getNumStatements(childNode2));
  cm.removeAllEstimatorPhases();
}

TEST_F(IPCGEstimatorPhaseBasic, ThreeNodeChildCall) {
  Config cfg;
  auto& cm = metacg::pgis::PiraMCGProcessor::get();
  auto& mcgm = metacg::graph::MCGManager::get();

  cm.setConfig(&cfg);
  cm.setNoOutput();
  mcgm.addToManagedGraphs("graph", std::make_unique<metacg::Callgraph>());
  auto mainNode = &mcgm.getCallgraph()->getOrInsertNode("main");
  attachAllMetaDataToGraph(mcgm.getCallgraph());
  pira::setPiraOneData(mainNode, 1, true);

  auto childNode = &mcgm.getCallgraph()->getOrInsertNode("child1");
  attachAllMetaDataToGraph(mcgm.getCallgraph());
  pira::setPiraOneData(childNode, 10, true);
  mcgm.getCallgraph()->addEdge(*mainNode, *childNode);

  auto childNode2 = &mcgm.getCallgraph()->getOrInsertNode("child2");
  attachAllMetaDataToGraph(mcgm.getCallgraph());
  pira::setPiraOneData(childNode2, 15, true);
  mcgm.getCallgraph()->addEdge(*mainNode, *childNode2);
  mcgm.getCallgraph()->addEdge(*childNode2, *childNode);

  cm.setCG(mcgm.getCallgraph());

  StatementCountEstimatorPhase sce(100, mcgm.getCallgraph());
  sce.setNoReport();
  cm.registerEstimatorPhase(&sce);
  cm.applyRegisteredPhases();
  auto graph = cm.getCallgraph(&cm);
  ASSERT_EQ(mainNode, graph->getMain());
  ASSERT_EQ(26, sce.getNumStatements(mainNode));
  ASSERT_EQ(10, sce.getNumStatements(childNode));
  ASSERT_EQ(25, sce.getNumStatements(childNode2));
  cm.removeAllEstimatorPhases();
}

TEST_F(IPCGEstimatorPhaseBasic, FourNodeCGwStmt) {
  Config cfg;
  auto& cm = metacg::pgis::PiraMCGProcessor::get();
  auto& mcgm = metacg::graph::MCGManager::get();

  cm.setConfig(&cfg);
  cm.setNoOutput();

  mcgm.addToManagedGraphs("graph", std::make_unique<metacg::Callgraph>());
  auto mainNode = &mcgm.getCallgraph()->getOrInsertNode("main");
  attachAllMetaDataToGraph(mcgm.getCallgraph());
  pira::setPiraOneData(mainNode, 12, true);

  auto childNode = &mcgm.getCallgraph()->getOrInsertNode("child1");
  attachAllMetaDataToGraph(mcgm.getCallgraph());
  pira::setPiraOneData(childNode, 7, true);
  mcgm.getCallgraph()->addEdge(*mainNode, *childNode);

  auto childNode2 = &mcgm.getCallgraph()->getOrInsertNode("child2");
  attachAllMetaDataToGraph(mcgm.getCallgraph());
  pira::setPiraOneData(childNode2, 42, true);
  mcgm.getCallgraph()->addEdge(*mainNode, *childNode2);

  auto childNode3 = &mcgm.getCallgraph()->getOrInsertNode("child3");
  attachAllMetaDataToGraph(mcgm.getCallgraph());
  pira::setPiraOneData(childNode3, 22, true);
  mcgm.getCallgraph()->addEdge(*childNode, *childNode3);

  cm.setCG(mcgm.getCallgraph());

  StatementCountEstimatorPhase sce(10, mcgm.getCallgraph());
  sce.setNoReport();
  cm.registerEstimatorPhase(&sce);
  cm.applyRegisteredPhases();
  auto graph = cm.getCallgraph(&cm);
  ASSERT_EQ(mainNode, graph->getMain());
  ASSERT_EQ(83, sce.getNumStatements(mainNode));
  ASSERT_EQ(29, sce.getNumStatements(childNode));
  ASSERT_EQ(42, sce.getNumStatements(childNode2));
  ASSERT_EQ(22, sce.getNumStatements(childNode3));
  cm.removeAllEstimatorPhases();
}

TEST_F(IPCGEstimatorPhaseBasic, FourNodeDiamondCGwStmt) {
  Config cfg;
  auto& cm = metacg::pgis::PiraMCGProcessor::get();
  auto& mcgm = metacg::graph::MCGManager::get();

  cm.setConfig(&cfg);
  cm.setNoOutput();
  mcgm.addToManagedGraphs("graph", std::make_unique<metacg::Callgraph>());
  auto mainNode = &mcgm.getCallgraph()->getOrInsertNode("main");
  attachAllMetaDataToGraph(mcgm.getCallgraph());
  pira::setPiraOneData(mainNode, 12, true);

  auto childNode = &mcgm.getCallgraph()->getOrInsertNode("child1");
  attachAllMetaDataToGraph(mcgm.getCallgraph());
  pira::setPiraOneData(childNode, 7, true);
  mcgm.getCallgraph()->addEdge(*mainNode, *childNode);

  auto childNode2 = &mcgm.getCallgraph()->getOrInsertNode("child2");
  attachAllMetaDataToGraph(mcgm.getCallgraph());
  pira::setPiraOneData(childNode2, 42, true);
  mcgm.getCallgraph()->addEdge(*mainNode, *childNode2);

  auto childNode3 = &mcgm.getCallgraph()->getOrInsertNode("child3");
  attachAllMetaDataToGraph(mcgm.getCallgraph());
  pira::setPiraOneData(childNode3, 22, true);
  mcgm.getCallgraph()->addEdge(*childNode, *childNode3);
  mcgm.getCallgraph()->addEdge(*childNode2, *childNode3);

  cm.setCG(mcgm.getCallgraph());

  StatementCountEstimatorPhase sce(10, mcgm.getCallgraph());
  sce.setNoReport();
  cm.registerEstimatorPhase(&sce);
  cm.applyRegisteredPhases();
  auto graph = cm.getCallgraph(&cm);
  ASSERT_EQ(mainNode, graph->getMain());
  ASSERT_EQ(83, sce.getNumStatements(mainNode));
  ASSERT_EQ(29, sce.getNumStatements(childNode));
  ASSERT_EQ(64, sce.getNumStatements(childNode2));
  ASSERT_EQ(22, sce.getNumStatements(childNode3));
  cm.removeAllEstimatorPhases();
}

/*
   o
  / \
  o  o
  \  /
   o
   |
   o
*/
TEST_F(IPCGEstimatorPhaseBasic, FiveNodeDiamondCGwStmt) {
  Config cfg;
  auto& cm = metacg::pgis::PiraMCGProcessor::get();
  auto& mcgm = metacg::graph::MCGManager::get();

  cm.setConfig(&cfg);
  cm.setNoOutput();
  mcgm.addToManagedGraphs("graph", std::make_unique<metacg::Callgraph>());
  auto mainNode = &mcgm.getCallgraph()->getOrInsertNode("main");
  attachAllMetaDataToGraph(mcgm.getCallgraph());
  pira::setPiraOneData(mainNode, 12, true);

  auto childNode = &mcgm.getCallgraph()->getOrInsertNode("child1");
  attachAllMetaDataToGraph(mcgm.getCallgraph());
  pira::setPiraOneData(childNode, 7, true);
  mcgm.getCallgraph()->addEdge(*mainNode, *childNode);

  auto childNode2 = &mcgm.getCallgraph()->getOrInsertNode("child2");
  attachAllMetaDataToGraph(mcgm.getCallgraph());
  pira::setPiraOneData(childNode2, 42, true);
  mcgm.getCallgraph()->addEdge(*mainNode, *childNode2);

  auto childNode3 = &mcgm.getCallgraph()->getOrInsertNode("child3");
  attachAllMetaDataToGraph(mcgm.getCallgraph());
  pira::setPiraOneData(childNode3, 22, true);
  mcgm.getCallgraph()->addEdge(*childNode, *childNode3);
  mcgm.getCallgraph()->addEdge(*childNode2, *childNode3);

  auto childNode4 = &mcgm.getCallgraph()->getOrInsertNode("child4");
  attachAllMetaDataToGraph(mcgm.getCallgraph());
  pira::setPiraOneData(childNode4, 4, true);
  mcgm.getCallgraph()->addEdge(*childNode3, *childNode4);

  cm.setCG(mcgm.getCallgraph());

  StatementCountEstimatorPhase sce(10, mcgm.getCallgraph());
  sce.setNoReport();
  cm.registerEstimatorPhase(&sce);
  cm.applyRegisteredPhases();
  auto graph = cm.getCallgraph(&cm);
  ASSERT_EQ(mainNode, graph->getMain());
  ASSERT_EQ(87, sce.getNumStatements(mainNode));
  ASSERT_EQ(33, sce.getNumStatements(childNode));
  ASSERT_EQ(68, sce.getNumStatements(childNode2));
  ASSERT_EQ(26, sce.getNumStatements(childNode3));
  ASSERT_EQ(4, sce.getNumStatements(childNode4));
  cm.removeAllEstimatorPhases();
}

class IPCGEstimatorPhaseTest : public ::testing::Test {
 protected:
  IPCGEstimatorPhaseTest() = default;
  /* Construct a call graph that has the following structure
   o
  / \ \
  o  o o
  \  / |
   o   o
   |   |
   o   o
*/
  static void createCalleeNode(const std::string& name, const std::string& caller, int numStatements, double runtime,
                               int numCalls) {
    auto& mcgm = metacg::graph::MCGManager::get();
    auto nodeCaller = &mcgm.getCallgraph()->getOrInsertNode(caller);
    auto nodeCallee = &mcgm.getCallgraph()->getOrInsertNode(name);
    attachAllMetaDataToGraph(mcgm.getCallgraph());
    pira::setPiraOneData(nodeCallee, numStatements, true);
    mcgm.getCallgraph()->addEdge(*nodeCaller, *nodeCallee);
  }

  static void attachAllMetaDataToGraph(metacg::Callgraph* cg) {
    metacg::pgis::attachMetaDataToGraph<pira::BaseProfileData>(cg);
    metacg::pgis::attachMetaDataToGraph<pira::PiraOneData>(cg);
    metacg::pgis::attachMetaDataToGraph<pira::PiraTwoData>(cg);
  }

  void SetUp() override {
    metacg::loggerutil::getLogger();
    auto& cm = metacg::pgis::PiraMCGProcessor::get();
    auto& mcgm = metacg::graph::MCGManager::get();
    mcgm.resetManager();
    mcgm.addToManagedGraphs("emptyGraph", std::make_unique<metacg::Callgraph>());

    cm.setConfig(new Config());
    cm.setNoOutput();
    auto mainNode = &mcgm.getCallgraph()->getOrInsertNode("main");
    attachAllMetaDataToGraph(mcgm.getCallgraph());
    pira::setPiraOneData(mainNode, 12, true);

    createCalleeNode("child1", "main", 7, 2.0, 100);
    createCalleeNode("child2", "main", 42, 23.0, 10);
    createCalleeNode("child3", "child1", 22, 14.0, 10);
    createCalleeNode("child3", "child2", 22, 14.0, 10);
    createCalleeNode("child4", "child3", 4, 11.2, 12);
    createCalleeNode("child5", "main", 15, 3.0, 12);
    createCalleeNode("child6", "child5", 2, 0.8, 1000);
    createCalleeNode("child7", "child6", 1, 20.2, 1002);

    cm.setCG(mcgm.getCallgraph());
  }
};

TEST_F(IPCGEstimatorPhaseTest, ValidateBasics) {
  auto& cm = metacg::pgis::PiraMCGProcessor::get();
  auto graph = cm.getCallgraph(&cm);
  ASSERT_NE(nullptr, graph->getMain());

  auto mainNode = graph->getMain();
  EXPECT_EQ(3, graph->getCallees(*mainNode).size());
  auto childNodes = graph->getCallees(*mainNode);
  for (const auto& elem : childNodes) {
    ASSERT_TRUE(elem->getFunctionName() == "child1" || elem->getFunctionName() == "child2" ||
                elem->getFunctionName() == "child5");
  }
  cm.removeAllEstimatorPhases();
}

TEST_F(IPCGEstimatorPhaseTest, ApplyPhaseFinalizesGraph) {
  auto& cm = metacg::pgis::PiraMCGProcessor::get();
  auto nep = std::make_unique<NopEstimatorPhase>(cm.getCallgraph());
  ASSERT_EQ(false, nep->didRun);
  cm.registerEstimatorPhase(nep.get());
  ASSERT_EQ(false, nep->didRun);
  cm.applyRegisteredPhases();
  ASSERT_EQ(true, nep->didRun);
  ASSERT_NE(0, cm.size());
  cm.removeAllEstimatorPhases();
}

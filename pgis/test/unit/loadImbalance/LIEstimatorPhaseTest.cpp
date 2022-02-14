/**
 * File: LIEstimatorPhaseTest.cpp
 * License: Part of the MetaCG project. Licensed under BSD 3 clause license. See LICENSE.txt file at
 * https://github.com/tudasc/metacg/LICENSE.txt
 */

#include "gtest/gtest.h"

#include "../LoggerUtil.h"
#include "CallgraphManager.h"
#include "MCGManager.h"
#include "loadImbalance/LIEstimatorPhase.h"

#include "loadImbalance/LIMetaData.h"
#include <memory>

class LIEstimatorPhaseTest : public ::testing::Test {
 protected:
  void SetUp() override { loggerutil::getLogger(); }
};

TEST_F(LIEstimatorPhaseTest, TrivialTest) { ASSERT_TRUE(true); }

/**
 * Tests whether an empty CG gets recognized, from test/IPCGEstimatorPhaseTest.cpp
 */
TEST_F(LIEstimatorPhaseTest, EmptyCG) {
  // XXX this is not ideal, but required for the death assert
  LOGGERUTIL_ENABLE_ERRORS_LOCAL

  Config cfg;
  auto &cm = metacg::pgis::PiraMCGProcessor::get();
  auto &mcgm = metacg::graph::MCGManager::get();
  mcgm.reset();
  cm.removeAllEstimatorPhases();
  cm.setConfig(&cfg);
  cm.setNoOutput();
  cm.setCG(mcgm.getCallgraph());

  auto liConfig = std::make_unique<LoadImbalance::LIConfig>(
      LoadImbalance::LIConfig{LoadImbalance::MetricType::Efficiency, 1.2, 0.1, LoadImbalance::ContextStrategy::None, 0,
                              LoadImbalance::ChildRelevanceStrategy::ConstantThreshold, 5, 0.0});
  LoadImbalance::LIEstimatorPhase lie(std::move(liConfig));
  cm.registerEstimatorPhase(&lie);
  ASSERT_TRUE(mcgm.getCallgraph().isEmpty());
  ASSERT_TRUE(cm.getCallgraph(&cm).isEmpty());
  ASSERT_DEATH(cm.applyRegisteredPhases(), "Running the processor on empty graph. Need to construct graph.");
}

TEST_F(LIEstimatorPhaseTest, AllCases) {
  Config cfg;

  auto &cm = metacg::pgis::PiraMCGProcessor::get();
  auto &mcgm = metacg::graph::MCGManager::get();
  mcgm.reset();
  cm.removeAllEstimatorPhases();
  cm.setConfig(&cfg);
  cm.setNoOutput();

  // setup graph
  // ===========

  // main node
  auto mainNode = mcgm.findOrCreateNode("main");
  mainNode->get<pira::BaseProfileData>()->setInclusiveRuntimeInSeconds(10.0);

  // irrelevant and balanced
  auto childNode1 = mcgm.findOrCreateNode("child1");
  childNode1->get<pira::BaseProfileData>()->setCallData(mainNode, 1, 0.2, 0.2, 0, 0);
  childNode1->get<pira::BaseProfileData>()->setCallData(mainNode, 1, 0.2, 0.2, 1, 0);
  childNode1->get<pira::BaseProfileData>()->setCallData(mainNode, 1, 0.2, 0.2, 2, 0);

  auto gc1 = mcgm.findOrCreateNode("gc1");
  childNode1->addChildNode(gc1);

  // irrelevant and imbalanced
  auto childNode2 = mcgm.findOrCreateNode("child2");
  childNode2->get<pira::BaseProfileData>()->setCallData(mainNode, 1, 0.0001, 0.001, 0, 0);
  childNode2->get<pira::BaseProfileData>()->setCallData(mainNode, 1, 0.2, 0.2, 1, 0);
  childNode2->get<pira::BaseProfileData>()->setCallData(mainNode, 1, 0.5, 0.5, 2, 0);

  auto gc2 = mcgm.findOrCreateNode("gc2");
  childNode2->addChildNode(gc2);

  // relevant and balanced
  auto childNode3 = mcgm.findOrCreateNode("child3");
  childNode3->get<pira::BaseProfileData>()->setCallData(mainNode, 1, 5.0, 5.0, 0, 0);
  childNode3->get<pira::BaseProfileData>()->setCallData(mainNode, 1, 5.0, 5.0, 1, 0);
  childNode3->get<pira::BaseProfileData>()->setCallData(mainNode, 1, 5.0, 5.0, 2, 0);

  auto gc3 = mcgm.findOrCreateNode("gc3");
  childNode3->addChildNode(gc3);

  // relevant and imbalanced
  auto childNode4 = mcgm.findOrCreateNode("child4");
  childNode4->get<pira::BaseProfileData>()->setCallData(mainNode, 1, 1.0, 1.0, 0, 0);
  childNode4->get<pira::BaseProfileData>()->setCallData(mainNode, 1, 5.0, 5.0, 1, 0);
  childNode4->get<pira::BaseProfileData>()->setCallData(mainNode, 1, 10.0, 10.0, 2, 0);

  auto gc4 = mcgm.findOrCreateNode("gc4");
  childNode4->addChildNode(gc4);

  auto gc5 = mcgm.findOrCreateNode("gc5");
  childNode4->addChildNode(gc5);

  auto childNode5 = mcgm.findOrCreateNode("child5");
  // no profiling data for child5

  mainNode->addChildNode(childNode1);
  mainNode->addChildNode(childNode2);
  mainNode->addChildNode(childNode3);
  mainNode->addChildNode(childNode4);
  childNode1->get<pira::PiraOneData>()->setComesFromCube();
  childNode2->get<pira::PiraOneData>()->setComesFromCube();
  childNode3->get<pira::PiraOneData>()->setComesFromCube();
  childNode4->get<pira::PiraOneData>()->setComesFromCube();
  childNode5->get<pira::PiraOneData>()->setComesFromCube();

  for (CgNodePtr n : mcgm.getCallgraph()) {
    n->get<pira::PiraOneData>()->setNumberOfStatements(100);
  }

  cm.setCG(mcgm.getCallgraph());

  // apply estimator phases
  auto liConfig = std::make_unique<LoadImbalance::LIConfig>(
      LoadImbalance::LIConfig{LoadImbalance::MetricType::Efficiency, 1.2, 0.1, LoadImbalance::ContextStrategy::None, 0,
                              LoadImbalance::ChildRelevanceStrategy::ConstantThreshold, 5, 0.0});
  LoadImbalance::LIEstimatorPhase lie(std::move(liConfig));
  cm.registerEstimatorPhase(&lie);
  cm.applyRegisteredPhases();

  auto graph = cm.getCallgraph(&cm);

  ASSERT_EQ(graph.findMain(), mainNode);

  ASSERT_EQ(graph.findMain()->isInstrumented(), true);
  ASSERT_EQ(graph.findNode("child1")->isInstrumented(), false);
  ASSERT_EQ(graph.findNode("child2")->isInstrumented(), false);
  ASSERT_EQ(graph.findNode("child3")->isInstrumented(), false);
  ASSERT_EQ(graph.findNode("child4")->isInstrumented(), true);
  ASSERT_EQ(graph.findNode("child5")->isInstrumented(), false);

  ASSERT_EQ(graph.findNode("gc1")->isInstrumented(), false);
  ASSERT_EQ(graph.findNode("gc2")->isInstrumented(), false);
  ASSERT_EQ(graph.findNode("gc3")->isInstrumented(), true);
  ASSERT_EQ(graph.findNode("gc4")->isInstrumented(), true);
  ASSERT_EQ(graph.findNode("gc5")->isInstrumented(), true);
}

TEST_F(LIEstimatorPhaseTest, Virtual) {
  Config cfg;
  auto &cm = metacg::pgis::PiraMCGProcessor::get();
  auto &mcgm = metacg::graph::MCGManager::get();
  mcgm.reset();
  cm.removeAllEstimatorPhases();
  ASSERT_TRUE(mcgm.getCallgraph().isEmpty());
  cm.setConfig(&cfg);
  cm.setNoOutput();

  // setup graph
  // ===========

  // main node
  auto mainNode = mcgm.findOrCreateNode("main");
  mainNode->get<pira::PiraOneData>()->setComesFromCube();
  mainNode->get<pira::BaseProfileData>()->setCallData(mainNode, 1, 10.0, 10.0, 0, 0);

  auto child = mcgm.findOrCreateNode("child");
  child->get<LoadImbalance::LIMetaData>()->setVirtual(true);

  auto grandchild = mcgm.findOrCreateNode("grandchild");

  auto grandgrandchild = mcgm.findOrCreateNode("grandgrandchild");

  mainNode->addChildNode(child);
  child->addChildNode(grandchild);
  grandchild->addChildNode(grandgrandchild);

  for (CgNodePtr n : mcgm.getCallgraph()) {
    n->get<pira::PiraOneData>()->setNumberOfStatements(100);
  }

  cm.setCG(mcgm.getCallgraph());

  // apply estimator phases
  auto liConfig = std::make_unique<LoadImbalance::LIConfig>(
      LoadImbalance::LIConfig{LoadImbalance::MetricType::Efficiency, 1.2, 0.1, LoadImbalance::ContextStrategy::None, 0,
                              LoadImbalance::ChildRelevanceStrategy::ConstantThreshold, 5, 0.0});
  LoadImbalance::LIEstimatorPhase lie(std::move(liConfig));
  cm.registerEstimatorPhase(&lie);
  cm.applyRegisteredPhases();

  auto graph = cm.getCallgraph(&cm);

  ASSERT_EQ(graph.findMain(), mainNode);
  ASSERT_EQ(graph.findNode("main")->isInstrumented(), true);
  ASSERT_EQ(graph.findNode("child")->isInstrumented(), true);
  ASSERT_EQ(graph.findNode("grandchild")->isInstrumented(), true);
  ASSERT_EQ(graph.findNode("grandgrandchild")->isInstrumented(), false);
}

TEST_F(LIEstimatorPhaseTest, AllPathsToMain) {
  Config cfg;
  auto &cm = metacg::pgis::PiraMCGProcessor::get();
  auto &mcgm = metacg::graph::MCGManager::get();
  mcgm.reset();
  cm.removeAllEstimatorPhases();
  cm.setConfig(&cfg);
  cm.setNoOutput();

  // setup graph
  // ===========
  // main node
  auto mainNode = mcgm.findOrCreateNode("main");
  mainNode->get<pira::PiraOneData>()->setComesFromCube();
  mainNode->get<pira::BaseProfileData>()->setCallData(mainNode, 1, 10.0, 10.0, 0, 0);

  auto child1 = mcgm.findOrCreateNode("child1");
  child1->get<LoadImbalance::LIMetaData>()->flag(LoadImbalance::FlagType::Irrelevant);
  mcgm.addEdge(mainNode, child1);

  auto child2 = mcgm.findOrCreateNode("child2");
  child2->get<LoadImbalance::LIMetaData>()->flag(LoadImbalance::FlagType::Irrelevant);
  mcgm.addEdge(mainNode, child2);

  auto grandchild = mcgm.findOrCreateNode("grandchild");
  mcgm.addEdge(child1, grandchild);
  mcgm.addEdge(child2, grandchild);

  for (CgNodePtr n : mcgm.getCallgraph()) {
    n->get<pira::PiraOneData>()->setNumberOfStatements(100);
  }

  grandchild->get<pira::PiraOneData>()->setComesFromCube();
  grandchild->get<pira::BaseProfileData>()->setCallData(child1, 1, 10.0, 1.0, 0, 0);
  grandchild->get<pira::BaseProfileData>()->setCallData(child1, 1, 10.0, 100.0, 0, 1);
  grandchild->get<pira::BaseProfileData>()->setCallData(child2, 1, 10.0, 1.0, 0, 0);
  grandchild->get<pira::BaseProfileData>()->setCallData(child2, 1, 10.0, 100.0, 0, 1);

  cm.setCG(mcgm.getCallgraph());

  // apply estimator phases
  auto liConfig = std::make_unique<LoadImbalance::LIConfig>(LoadImbalance::LIConfig{
      LoadImbalance::MetricType::Efficiency, 1.2, 0.1, LoadImbalance::ContextStrategy::AllPathsToMain, 0,
      LoadImbalance::ChildRelevanceStrategy::ConstantThreshold, 5, 0.0});
  LoadImbalance::LIEstimatorPhase lie(std::move(liConfig));
  cm.registerEstimatorPhase(&lie);
  cm.applyRegisteredPhases();

  auto graph = cm.getCallgraph(&cm);

  ASSERT_EQ(graph.findMain(), mainNode);
  ASSERT_EQ(graph.findNode("main")->isInstrumented(), true);
  ASSERT_EQ(graph.findNode("child1")->isInstrumented(), true);
  ASSERT_EQ(graph.findNode("child2")->isInstrumented(), true);
  ASSERT_EQ(graph.findNode("grandchild")->isInstrumented(), true);
}

TEST_F(LIEstimatorPhaseTest, MajorPathsToMain) {
  Config cfg;
  auto &cm = metacg::pgis::PiraMCGProcessor::get();
  auto &mcgm = metacg::graph::MCGManager::get();
  mcgm.reset();
  cm.removeAllEstimatorPhases();
  cm.setConfig(&cfg);
  cm.setNoOutput();

  // setup graph
  // ===========
  // main node
  auto mainNode = mcgm.findOrCreateNode("main");
  mainNode->get<pira::PiraOneData>()->setComesFromCube();
  mainNode->get<pira::BaseProfileData>()->setCallData(mainNode, 1, 10.0, 10.0, 0, 0);

  auto child1 = mcgm.findOrCreateNode("child1");
  mcgm.addEdge(mainNode, child1);

  auto child2 = mcgm.findOrCreateNode("child2");
  child2->get<LoadImbalance::LIMetaData>()->flag(LoadImbalance::FlagType::Irrelevant);
  mcgm.addEdge(mainNode, child2);

  auto grandchild = mcgm.findOrCreateNode("grandchild");
  mcgm.addEdge(child1, grandchild);
  mcgm.addEdge(child2, grandchild);

  for (CgNodePtr n : mcgm.getCallgraph()) {
    n->get<pira::PiraOneData>()->setNumberOfStatements(100);
  }

  grandchild->get<pira::PiraOneData>()->setComesFromCube();
  grandchild->get<pira::BaseProfileData>()->setCallData(child1, 1, 10.0, 1.0, 0, 0);
  grandchild->get<pira::BaseProfileData>()->setCallData(child1, 1, 10.0, 100.0, 0, 1);
  grandchild->get<pira::BaseProfileData>()->setCallData(child2, 1, 10.0, 1.0, 0, 0);
  grandchild->get<pira::BaseProfileData>()->setCallData(child2, 1, 10.0, 100.0, 0, 1);

  cm.setCG(mcgm.getCallgraph());

  // apply estimator phases
  auto liConfig = std::make_unique<LoadImbalance::LIConfig>(
      LoadImbalance::LIConfig{LoadImbalance::MetricType::Efficiency, 1.2, 0.1, LoadImbalance::ContextStrategy::None, 0,
                              LoadImbalance::ChildRelevanceStrategy::ConstantThreshold, 5, 0.0});
  LoadImbalance::LIEstimatorPhase lie(std::move(liConfig));
  cm.registerEstimatorPhase(&lie);
  cm.applyRegisteredPhases();

  auto graph = cm.getCallgraph(&cm);

  ASSERT_EQ(graph.findMain(), mainNode);
  ASSERT_EQ(graph.findNode("main")->isInstrumented(), true);
  ASSERT_EQ(graph.findNode("child1")->isInstrumented(), true);
  ASSERT_EQ(graph.findNode("child2")->isInstrumented(), false);
  ASSERT_EQ(graph.findNode("grandchild")->isInstrumented(), true);
}

TEST_F(LIEstimatorPhaseTest, MajorParentSteps) {
  Config cfg;
  auto &cm = metacg::pgis::PiraMCGProcessor::get();
  auto &mcgm = metacg::graph::MCGManager::get();
  mcgm.reset();
  cm.removeAllEstimatorPhases();
  cm.setConfig(&cfg);
  cm.setNoOutput();

  // setup graph
  // ===========
  // main node
  auto mainNode = mcgm.findOrCreateNode("main");
  mainNode->get<pira::PiraOneData>()->setComesFromCube();
  mainNode->get<pira::BaseProfileData>()->setCallData(mainNode, 1, 10.0, 10.0, 0, 0);

  auto child1 = mcgm.findOrCreateNode("child1");
  child1->get<LoadImbalance::LIMetaData>()->flag(LoadImbalance::FlagType::Visited);
  mcgm.addEdge(mainNode, child1);

  auto child2 = mcgm.findOrCreateNode("child2");
  child2->get<LoadImbalance::LIMetaData>()->flag(LoadImbalance::FlagType::Visited);
  child1->addChildNode(child2);  // TODO: Necessary? Remove me!
  mcgm.addEdge(child1, child2);

  auto child3 = mcgm.findOrCreateNode("child3");
  child3->get<LoadImbalance::LIMetaData>()->flag(LoadImbalance::FlagType::Visited);
  mcgm.addEdge(child2, child3);

  child3->get<pira::PiraOneData>()->setComesFromCube();
  child3->get<pira::BaseProfileData>()->setCallData(mainNode, 1, 10.0, 1.0, 0, 0);
  child3->get<pira::BaseProfileData>()->setCallData(mainNode, 1, 10.0, 100.0, 0, 1);

  cm.setCG(mcgm.getCallgraph());
  // apply estimator phases
  auto liConfig = std::make_unique<LoadImbalance::LIConfig>(LoadImbalance::LIConfig{
      LoadImbalance::MetricType::Efficiency, 1.2, 0.1, LoadImbalance::ContextStrategy::MajorParentSteps, 1,
      LoadImbalance::ChildRelevanceStrategy::ConstantThreshold, 5, 0.0});
  LoadImbalance::LIEstimatorPhase lie(std::move(liConfig));
  cm.registerEstimatorPhase(&lie);
  cm.applyRegisteredPhases();

  auto graph = cm.getCallgraph(&cm);

  ASSERT_EQ(graph.findMain(), mainNode);
  ASSERT_EQ(graph.findNode("main")->isInstrumented(), true);
  ASSERT_EQ(graph.findNode("child1")->isInstrumented(), false);
  ASSERT_EQ(graph.findNode("child2")->isInstrumented(), true);
  ASSERT_EQ(graph.findNode("child3")->isInstrumented(), true);
}

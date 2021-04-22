/**
 * File: LIEstimatorPhaseTest.cpp
 * License: Part of the MetaCG project. Licensed under BSD 3 clause license. See LICENSE.txt file at https://github.com/tudasc/metacg/LICENSE.txt
 */

#include "gtest/gtest.h"

#include "../LoggerUtil.h"
#include "CallgraphManager.h"
#include "loadImbalance/LIEstimatorPhase.h"
#include "loadImbalance/LIMetaData.h"

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
  auto &cm = CallgraphManager::get();
  cm.clear();
  cm.setConfig(&cfg);
  cm.setNoOutput();

  LoadImbalance::Config liConfig = {LoadImbalance::MetricType::Efficiency,
                                     1.2,
                                     0.1,
                                    LoadImbalance::ContextStrategy::None,
                                     0,
                                    LoadImbalance::ChildRelevanceStrategy::ConstantThreshold,
                                     5,
                                     0.0};

  LoadImbalance::LIEstimatorPhase lie(liConfig);
  cm.registerEstimatorPhase(&lie);
  ASSERT_DEATH(cm.applyRegisteredPhases(), "CallgraphManager: Cannot find main function.");
}

TEST_F(LIEstimatorPhaseTest, AllCases) {
  Config cfg;

  auto &cm = CallgraphManager::get();
  cm.clear();
  cm.setConfig(&cfg);

  //spdlog::set_level(spdlog::level::debug);
  cm.setNoOutput();

  // setup graph
  // ===========

  // main node
  auto mainNode = cm.findOrCreateNode("main", 1.2);
  mainNode->get<pira::BaseProfileData>()->setInclusiveRuntimeInSeconds(10.0);

  // irrelevant and balanced
  auto childNode1 = cm.findOrCreateNode("child1");
  childNode1->get<pira::BaseProfileData>()->addCallData(mainNode, 1, 0.2, 0.2, 0, 0);
  childNode1->get<pira::BaseProfileData>()->addCallData(mainNode, 1, 0.2, 0.2, 1, 0);
  childNode1->get<pira::BaseProfileData>()->addCallData(mainNode, 1, 0.2, 0.2, 2, 0);

  auto gc1 = cm.findOrCreateNode("gc1");
  childNode1->addChildNode(gc1);

  // irrelevant and imbalanced
  auto childNode2 = cm.findOrCreateNode("child2");
  childNode2->get<pira::BaseProfileData>()->addCallData(mainNode, 1, 0.0001, 0.001, 0, 0);
  childNode2->get<pira::BaseProfileData>()->addCallData(mainNode, 1, 0.2, 0.2, 1, 0);
  childNode2->get<pira::BaseProfileData>()->addCallData(mainNode, 1, 0.5, 0.5, 2, 0);

  auto gc2 = cm.findOrCreateNode("gc2");
  childNode2->addChildNode(gc2);

  // relevant and balanced
  auto childNode3 = cm.findOrCreateNode("child3");
  childNode3->get<pira::BaseProfileData>()->addCallData(mainNode, 1, 5.0, 5.0, 0, 0);
  childNode3->get<pira::BaseProfileData>()->addCallData(mainNode, 1, 5.0, 5.0, 1, 0);
  childNode3->get<pira::BaseProfileData>()->addCallData(mainNode, 1, 5.0, 5.0, 2, 0);

  auto gc3 = cm.findOrCreateNode("gc3");
  childNode3->addChildNode(gc3);

  // relevant and imbalanced
  auto childNode4 = cm.findOrCreateNode("child4");
  childNode4->get<pira::BaseProfileData>()->addCallData(mainNode, 1, 1.0, 1.0, 0, 0);
  childNode4->get<pira::BaseProfileData>()->addCallData(mainNode, 1, 5.0, 5.0, 1, 0);
  childNode4->get<pira::BaseProfileData>()->addCallData(mainNode, 1, 10.0, 10.0, 2, 0);

  auto gc4 = cm.findOrCreateNode("gc4");
  childNode4->addChildNode(gc4);

  auto gc5 = cm.findOrCreateNode("gc5");
  childNode4->addChildNode(gc5);

  auto childNode5 = cm.findOrCreateNode("child5");
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

  for (CgNodePtr n : cm.getCallgraph(&cm)) {
    n->get<pira::PiraOneData>()->setNumberOfStatements(100);
  }

  //std::cout << cm.getCallgraph(&cm).findMain()->getFunctionName() << std::endl;

  // apply estimator phases
  LoadImbalance::Config liConfig = {LoadImbalance::MetricType::Efficiency,
                                     1.2,
                                     0.1,
                                    LoadImbalance::ContextStrategy::None,
                                     0,
                                    LoadImbalance::ChildRelevanceStrategy::ConstantThreshold,
                                     5,
                                     0.0};
  LoadImbalance::LIEstimatorPhase lie(liConfig);
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
  auto &cm = CallgraphManager::get();
  cm.clear();
  cm.setConfig(&cfg);
  cm.setNoOutput();

  // setup graph
  // ===========

  // main node
  auto mainNode = cm.findOrCreateNode("main", 100);
  mainNode->get<pira::PiraOneData>()->setComesFromCube();
  mainNode->get<pira::BaseProfileData>()->addCallData(mainNode, 1, 10.0, 10.0, 0, 0);

  auto child = cm.findOrCreateNode("child", 100);
  child->get<LoadImbalance::LIMetaData>()->setVirtual(true);

  auto grandchild = cm.findOrCreateNode("grandchild", 100);

  auto grandgrandchild = cm.findOrCreateNode("grandgrandchild", 100);

  mainNode->addChildNode(child);
  child->addChildNode(grandchild);
  grandchild->addChildNode(grandgrandchild);

  for (CgNodePtr n : cm.getCallgraph(&cm)) {
    n->get<pira::PiraOneData>()->setNumberOfStatements(100);
  }

  // apply estimator phases
  LoadImbalance::Config liConfig = {LoadImbalance::MetricType::Efficiency,
                                     1.2,
                                     0.1,
                                    LoadImbalance::ContextStrategy::None,
                                     0,
                                    LoadImbalance::ChildRelevanceStrategy::ConstantThreshold,
                                     5,
                                     0.0};
  LoadImbalance::LIEstimatorPhase lie(liConfig);
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
  auto &cm = CallgraphManager::get();
  cm.clear();
  cm.setConfig(&cfg);
  cm.setNoOutput();

  // setup graph
  // ===========
  // main node
  auto mainNode = cm.findOrCreateNode("main", 100);
  mainNode->get<pira::PiraOneData>()->setComesFromCube();
  mainNode->get<pira::BaseProfileData>()->addCallData(mainNode, 1, 10.0, 10.0, 0, 0);

  auto child1 = cm.findOrCreateNode("child1", 100);
  child1->get<LoadImbalance::LIMetaData>()->flag(LoadImbalance::FlagType::Irrelevant);
  cm.putEdge("main", "child1");

  auto child2 = cm.findOrCreateNode("child2", 100);
  child2->get<LoadImbalance::LIMetaData>()->flag(LoadImbalance::FlagType::Irrelevant);
  cm.putEdge("main", "child2");

  auto grandchild = cm.findOrCreateNode("grandchild", 100);
  cm.putEdge("child1", "grandchild");
  cm.putEdge("child2", "grandchild");

  for (CgNodePtr n : cm.getCallgraph(&cm)) {
    n->get<pira::PiraOneData>()->setNumberOfStatements(100);
  }

  grandchild->get<pira::PiraOneData>()->setComesFromCube();
  grandchild->get<pira::BaseProfileData>()->addCallData(child1, 1, 10.0, 1.0, 0, 0);
  grandchild->get<pira::BaseProfileData>()->addCallData(child1, 1, 10.0, 100.0, 0, 1);
  grandchild->get<pira::BaseProfileData>()->addCallData(child2, 1, 10.0, 1.0, 0, 0);
  grandchild->get<pira::BaseProfileData>()->addCallData(child2, 1, 10.0, 100.0, 0, 1);

  //std::cout << grandchild->getParentNodes() << std::endl;

  // apply estimator phases
  LoadImbalance::Config liConfig = {LoadImbalance::MetricType::Efficiency,
                                    1.2,
                                    0.1,
                                    LoadImbalance::ContextStrategy::AllPathsToMain,
                                    0,
                                    LoadImbalance::ChildRelevanceStrategy::ConstantThreshold,
                                    5,
                                    0.0};
  LoadImbalance::LIEstimatorPhase lie(liConfig);
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
  auto &cm = CallgraphManager::get();
  cm.clear();
  cm.setConfig(&cfg);
  cm.setNoOutput();

  // setup graph
  // ===========
  // main node
  auto mainNode = cm.findOrCreateNode("main", 100);
  mainNode->get<pira::PiraOneData>()->setComesFromCube();
  mainNode->get<pira::BaseProfileData>()->addCallData(mainNode, 1, 10.0, 10.0, 0, 0);

  auto child1 = cm.findOrCreateNode("child1", 100);
  cm.putEdge("main", "child1");

  auto child2 = cm.findOrCreateNode("child2", 100);
  child2->get<LoadImbalance::LIMetaData>()->flag(LoadImbalance::FlagType::Irrelevant);
  cm.putEdge("main", "child2");

  auto grandchild = cm.findOrCreateNode("grandchild", 100);
  cm.putEdge("child1", "grandchild");
  cm.putEdge("child2", "grandchild");

  for (CgNodePtr n : cm.getCallgraph(&cm)) {
    n->get<pira::PiraOneData>()->setNumberOfStatements(100);
  }

  grandchild->get<pira::PiraOneData>()->setComesFromCube();
  grandchild->get<pira::BaseProfileData>()->addCallData(child1, 1, 10.0, 1.0, 0, 0);
  grandchild->get<pira::BaseProfileData>()->addCallData(child1, 1, 10.0, 100.0, 0, 1);
  grandchild->get<pira::BaseProfileData>()->addCallData(child2, 1, 10.0, 1.0, 0, 0);
  grandchild->get<pira::BaseProfileData>()->addCallData(child2, 1, 10.0, 100.0, 0, 1);

  //std::cout << grandchild->getParentNodes() << std::endl;

  // apply estimator phases
  LoadImbalance::Config liConfig = {LoadImbalance::MetricType::Efficiency,
                                    1.2,
                                    0.1,
                                    LoadImbalance::ContextStrategy::MajorPathsToMain,
                                    0,
                                    LoadImbalance::ChildRelevanceStrategy::ConstantThreshold,
                                    5,
                                    0.0};
  LoadImbalance::LIEstimatorPhase lie(liConfig);
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
  auto &cm = CallgraphManager::get();
  cm.clear();
  cm.setConfig(&cfg);
  cm.setNoOutput();

  // setup graph
  // ===========
  // main node
  auto mainNode = cm.findOrCreateNode("main", 100);
  mainNode->get<pira::PiraOneData>()->setComesFromCube();
  mainNode->get<pira::BaseProfileData>()->addCallData(mainNode, 1, 10.0, 10.0, 0, 0);

  auto child1 = cm.findOrCreateNode("child1", 100);
  child1->get<LoadImbalance::LIMetaData>()->flag(LoadImbalance::FlagType::Visited);
  cm.putEdge("main", "child1");

  auto child2 = cm.findOrCreateNode("child2", 100);
  child2->get<LoadImbalance::LIMetaData>()->flag(LoadImbalance::FlagType::Visited);
  child1->addChildNode(child2);
  cm.putEdge("child1", "child2");

  auto child3 = cm.findOrCreateNode("child3", 100);
  child3->get<LoadImbalance::LIMetaData>()->flag(LoadImbalance::FlagType::Visited);
  cm.putEdge("child2", "child3");

  child3->get<pira::PiraOneData>()->setComesFromCube();
  child3->get<pira::BaseProfileData>()->addCallData(mainNode, 1, 10.0, 1.0, 0, 0);
  child3->get<pira::BaseProfileData>()->addCallData(mainNode, 1, 10.0, 100.0, 0, 1);



  // apply estimator phases
  LoadImbalance::Config liConfig = {LoadImbalance::MetricType::Efficiency,
                                    1.2,
                                    0.1,
                                    LoadImbalance::ContextStrategy::MajorParentSteps,
                                    1,
                                    LoadImbalance::ChildRelevanceStrategy::ConstantThreshold,
                                    5,
                                    0.0};
  LoadImbalance::LIEstimatorPhase lie(liConfig);
  cm.registerEstimatorPhase(&lie);
  cm.applyRegisteredPhases();

  auto graph = cm.getCallgraph(&cm);

  ASSERT_EQ(graph.findMain(), mainNode);
  ASSERT_EQ(graph.findNode("main")->isInstrumented(), true);
  ASSERT_EQ(graph.findNode("child1")->isInstrumented(), false);
  ASSERT_EQ(graph.findNode("child2")->isInstrumented(), true);
  ASSERT_EQ(graph.findNode("child3")->isInstrumented(), true);
}

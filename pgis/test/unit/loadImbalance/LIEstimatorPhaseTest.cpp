/**
 * File: LIEstimatorPhaseTest.cpp
 * License: Part of the MetaCG project. Licensed under BSD 3 clause license. See LICENSE.txt file at
 * https://github.com/tudasc/metacg/LICENSE.txt
 */

#include "gtest/gtest.h"

#include "LoggerUtil.h"
#include "MCGManager.h"
#include "MetaData/PGISMetaData.h"
#include "PiraMCGProcessor.h"
#include "loadImbalance/LIEstimatorPhase.h"
#include "loadImbalance/LIMetaData.h"
#include <memory>

class LIEstimatorPhaseTest : public ::testing::Test {
 protected:
  void SetUp() override { metacg::loggerutil::getLogger(); }

  static void attachAllMetaDataToGraph(metacg::Callgraph *cg) {
    pgis::attachMetaDataToGraph<pira::BaseProfileData>(cg);
    pgis::attachMetaDataToGraph<pira::PiraOneData>(cg);
    pgis::attachMetaDataToGraph<pira::PiraTwoData>(cg);
    pgis::attachMetaDataToGraph<LoadImbalance::LIMetaData>(cg);
  }
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
  mcgm.resetManager();
  mcgm.addToManagedGraphs("emptyGraph", std::make_unique<metacg::Callgraph>());
  cm.removeAllEstimatorPhases();
  cm.setConfig(&cfg);
  cm.setNoOutput();
  cm.setCG(mcgm.getCallgraph());

  auto liConfig = std::make_unique<LoadImbalance::LIConfig>(
      LoadImbalance::LIConfig{LoadImbalance::MetricType::Efficiency, 1.2, 0.1, LoadImbalance::ContextStrategy::None, 0,
                              LoadImbalance::ChildRelevanceStrategy::ConstantThreshold, 5, 0.0});
  LoadImbalance::LIEstimatorPhase lie(std::move(liConfig), cm.getCallgraph());
  cm.registerEstimatorPhase(&lie);
  ASSERT_TRUE(mcgm.getCallgraph()->isEmpty());
  ASSERT_TRUE(cm.getCallgraph(&cm)->isEmpty());
  ASSERT_DEATH(cm.applyRegisteredPhases(), "Running the processor on empty graph. Need to construct graph.");
}

TEST_F(LIEstimatorPhaseTest, AllCases) {
  Config cfg;

  auto &cm = metacg::pgis::PiraMCGProcessor::get();
  auto &mcgm = metacg::graph::MCGManager::get();
  mcgm.resetManager();
  mcgm.addToManagedGraphs("emptyGraph", std::make_unique<metacg::Callgraph>());
  cm.removeAllEstimatorPhases();
  cm.setConfig(&cfg);
  cm.setNoOutput();

  // setup graph
  // ===========

  // main node
  auto mainNode = mcgm.getCallgraph()->getOrInsertNode("main");
  attachAllMetaDataToGraph(mcgm.getCallgraph());
  mainNode->get<pira::BaseProfileData>()->setInclusiveRuntimeInSeconds(10.0);

  // irrelevant and balanced
  auto childNode1 = mcgm.getCallgraph()->getOrInsertNode("child1");
  attachAllMetaDataToGraph(mcgm.getCallgraph());
  childNode1->get<pira::BaseProfileData>()->setCallData(mainNode, 1, 0.2, 0.2, 0, 0);
  childNode1->get<pira::BaseProfileData>()->setCallData(mainNode, 1, 0.2, 0.2, 1, 0);
  childNode1->get<pira::BaseProfileData>()->setCallData(mainNode, 1, 0.2, 0.2, 2, 0);

  auto gc1 = mcgm.getCallgraph()->getOrInsertNode("gc1");
  mcgm.getCallgraph()->addEdge(childNode1, gc1);

  // irrelevant and imbalanced
  auto childNode2 = mcgm.getCallgraph()->getOrInsertNode("child2");
  attachAllMetaDataToGraph(mcgm.getCallgraph());
  childNode2->get<pira::BaseProfileData>()->setCallData(mainNode, 1, 0.0001, 0.001, 0, 0);
  childNode2->get<pira::BaseProfileData>()->setCallData(mainNode, 1, 0.2, 0.2, 1, 0);
  childNode2->get<pira::BaseProfileData>()->setCallData(mainNode, 1, 0.5, 0.5, 2, 0);

  auto gc2 = mcgm.getCallgraph()->getOrInsertNode("gc2");
  mcgm.getCallgraph()->addEdge(childNode2, gc2);

  // relevant and balanced
  auto childNode3 = mcgm.getCallgraph()->getOrInsertNode("child3");
  attachAllMetaDataToGraph(mcgm.getCallgraph());
  childNode3->get<pira::BaseProfileData>()->setCallData(mainNode, 1, 5.0, 5.0, 0, 0);
  childNode3->get<pira::BaseProfileData>()->setCallData(mainNode, 1, 5.0, 5.0, 1, 0);
  childNode3->get<pira::BaseProfileData>()->setCallData(mainNode, 1, 5.0, 5.0, 2, 0);

  auto gc3 = mcgm.getCallgraph()->getOrInsertNode("gc3");
  mcgm.getCallgraph()->addEdge(childNode3, gc3);

  // relevant and imbalanced
  auto childNode4 = mcgm.getCallgraph()->getOrInsertNode("child4");
  attachAllMetaDataToGraph(mcgm.getCallgraph());
  childNode4->get<pira::BaseProfileData>()->setCallData(mainNode, 1, 1.0, 1.0, 0, 0);
  childNode4->get<pira::BaseProfileData>()->setCallData(mainNode, 1, 5.0, 5.0, 1, 0);
  childNode4->get<pira::BaseProfileData>()->setCallData(mainNode, 1, 10.0, 10.0, 2, 0);

  auto gc4 = mcgm.getCallgraph()->getOrInsertNode("gc4");
  mcgm.getCallgraph()->addEdge(childNode4, gc4);

  auto gc5 = mcgm.getCallgraph()->getOrInsertNode("gc5");
  mcgm.getCallgraph()->addEdge(childNode4, gc5);

  auto childNode5 = mcgm.getCallgraph()->getOrInsertNode("child5");
  // no profiling data for child5
  attachAllMetaDataToGraph(mcgm.getCallgraph());

  mcgm.getCallgraph()->addEdge(mainNode, childNode1);
  mcgm.getCallgraph()->addEdge(mainNode, childNode2);
  mcgm.getCallgraph()->addEdge(mainNode, childNode3);
  mcgm.getCallgraph()->addEdge(mainNode, childNode4);
  childNode1->get<pira::PiraOneData>()->setComesFromCube();
  childNode2->get<pira::PiraOneData>()->setComesFromCube();
  childNode3->get<pira::PiraOneData>()->setComesFromCube();
  childNode4->get<pira::PiraOneData>()->setComesFromCube();
  childNode5->get<pira::PiraOneData>()->setComesFromCube();

  for (const auto &elem : mcgm.getCallgraph()->getNodes()) {
    const auto &n = elem.second.get();
    n->get<pira::PiraOneData>()->setNumberOfStatements(100);
  }

  cm.setCG(mcgm.getCallgraph());

  // apply estimator phases
  auto liConfig = std::make_unique<LoadImbalance::LIConfig>(
      LoadImbalance::LIConfig{LoadImbalance::MetricType::Efficiency, 1.2, 0.1, LoadImbalance::ContextStrategy::None, 0,
                              LoadImbalance::ChildRelevanceStrategy::ConstantThreshold, 5, 0.0});
  LoadImbalance::LIEstimatorPhase lie(std::move(liConfig), cm.getCallgraph());
  cm.registerEstimatorPhase(&lie);
  cm.applyRegisteredPhases();

  auto graph = cm.getCallgraph(&cm);

  ASSERT_EQ(graph->getMain(), mainNode);

  ASSERT_EQ(pgis::isInstrumented(graph->getMain()), true);
  ASSERT_EQ(pgis::isInstrumented(graph->getNode("child1")), false);
  ASSERT_EQ(pgis::isInstrumented(graph->getNode("child2")), false);
  ASSERT_EQ(pgis::isInstrumented(graph->getNode("child3")), false);
  ASSERT_EQ(pgis::isInstrumented(graph->getNode("child4")), true);
  ASSERT_EQ(pgis::isInstrumented(graph->getNode("child5")), false);

  ASSERT_EQ(pgis::isInstrumented(graph->getNode("gc1")), false);
  ASSERT_EQ(pgis::isInstrumented(graph->getNode("gc2")), false);
  ASSERT_EQ(pgis::isInstrumented(graph->getNode("gc3")), true);
  ASSERT_EQ(pgis::isInstrumented(graph->getNode("gc4")), true);
  ASSERT_EQ(pgis::isInstrumented(graph->getNode("gc5")), true);
}

TEST_F(LIEstimatorPhaseTest, Virtual) {
  Config cfg;
  auto &cm = metacg::pgis::PiraMCGProcessor::get();
  auto &mcgm = metacg::graph::MCGManager::get();
  mcgm.resetManager();
  mcgm.addToManagedGraphs("emptyGraph", std::make_unique<metacg::Callgraph>());
  cm.removeAllEstimatorPhases();
  ASSERT_TRUE(mcgm.getCallgraph()->isEmpty());
  cm.setConfig(&cfg);
  cm.setNoOutput();

  // setup graph
  // ===========

  // main node
  auto mainNode = mcgm.getCallgraph()->getOrInsertNode("main");
  attachAllMetaDataToGraph(mcgm.getCallgraph());
  mainNode->get<pira::PiraOneData>()->setComesFromCube();
  mainNode->get<pira::BaseProfileData>()->setCallData(mainNode, 1, 10.0, 10.0, 0, 0);

  auto child = mcgm.getCallgraph()->getOrInsertNode("child");
  attachAllMetaDataToGraph(mcgm.getCallgraph());
  child->get<LoadImbalance::LIMetaData>()->setVirtual(true);

  auto grandchild = mcgm.getCallgraph()->getOrInsertNode("grandchild");

  auto grandgrandchild = mcgm.getCallgraph()->getOrInsertNode("grandgrandchild");

  mcgm.getCallgraph()->addEdge(mainNode, child);
  mcgm.getCallgraph()->addEdge(child, grandchild);
  mcgm.getCallgraph()->addEdge(grandchild, grandgrandchild);
  attachAllMetaDataToGraph(mcgm.getCallgraph());
  for (const auto &elem : mcgm.getCallgraph()->getNodes()) {
    const auto &n = elem.second.get();
    n->get<pira::PiraOneData>()->setNumberOfStatements(100);
  }
  cm.setCG(mcgm.getCallgraph());

  // apply estimator phases
  auto liConfig = std::make_unique<LoadImbalance::LIConfig>(
      LoadImbalance::LIConfig{LoadImbalance::MetricType::Efficiency, 1.2, 0.1, LoadImbalance::ContextStrategy::None, 0,
                              LoadImbalance::ChildRelevanceStrategy::ConstantThreshold, 5, 0.0});
  LoadImbalance::LIEstimatorPhase lie(std::move(liConfig), cm.getCallgraph());
  cm.registerEstimatorPhase(&lie);
  cm.applyRegisteredPhases();

  auto graph = cm.getCallgraph(&cm);

  ASSERT_EQ(graph->getMain(), mainNode);
  ASSERT_EQ(pgis::isInstrumented(graph->getNode("main")), true);
  ASSERT_EQ(pgis::isInstrumented(graph->getNode("child")), true);
  ASSERT_EQ(pgis::isInstrumented(graph->getNode("grandchild")), true);
  ASSERT_EQ(pgis::isInstrumented(graph->getNode("grandgrandchild")), false);
}

TEST_F(LIEstimatorPhaseTest, AllPathsToMain) {
  Config cfg;
  auto &cm = metacg::pgis::PiraMCGProcessor::get();
  auto &mcgm = metacg::graph::MCGManager::get();
  mcgm.resetManager();
  mcgm.addToManagedGraphs("emptyGraph", std::make_unique<metacg::Callgraph>());
  cm.removeAllEstimatorPhases();
  cm.setConfig(&cfg);
  cm.setNoOutput();

  // setup graph
  // ===========
  // main node
  auto mainNode = mcgm.getCallgraph()->getOrInsertNode("main");
  attachAllMetaDataToGraph(mcgm.getCallgraph());
  mainNode->get<pira::PiraOneData>()->setComesFromCube();
  mainNode->get<pira::BaseProfileData>()->setCallData(mainNode, 1, 10.0, 10.0, 0, 0);

  auto child1 = mcgm.getCallgraph()->getOrInsertNode("child1");
  attachAllMetaDataToGraph(mcgm.getCallgraph());
  child1->get<LoadImbalance::LIMetaData>()->flag(LoadImbalance::FlagType::Irrelevant);
  mcgm.getCallgraph()->addEdge(mainNode, child1);

  auto child2 = mcgm.getCallgraph()->getOrInsertNode("child2");
  attachAllMetaDataToGraph(mcgm.getCallgraph());
  child2->get<LoadImbalance::LIMetaData>()->flag(LoadImbalance::FlagType::Irrelevant);
  mcgm.getCallgraph()->addEdge(mainNode, child2);

  auto grandchild = mcgm.getCallgraph()->getOrInsertNode("grandchild");
  mcgm.getCallgraph()->addEdge(child1, grandchild);
  mcgm.getCallgraph()->addEdge(child2, grandchild);

  attachAllMetaDataToGraph(mcgm.getCallgraph());
  for (const auto &elem : mcgm.getCallgraph()->getNodes()) {
    const auto &n = elem.second.get();
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
  LoadImbalance::LIEstimatorPhase lie(std::move(liConfig), cm.getCallgraph());
  cm.registerEstimatorPhase(&lie);
  cm.applyRegisteredPhases();

  auto graph = cm.getCallgraph(&cm);

  ASSERT_EQ(graph->getMain(), mainNode);
  ASSERT_EQ(pgis::isInstrumented(graph->getNode("main")), true);
  ASSERT_EQ(pgis::isInstrumented(graph->getNode("child1")), true);
  ASSERT_EQ(pgis::isInstrumented(graph->getNode("child2")), true);
  ASSERT_EQ(pgis::isInstrumented(graph->getNode("grandchild")), true);
}

TEST_F(LIEstimatorPhaseTest, MajorPathsToMain) {
  Config cfg;
  auto &cm = metacg::pgis::PiraMCGProcessor::get();
  auto &mcgm = metacg::graph::MCGManager::get();
  mcgm.resetManager();
  mcgm.addToManagedGraphs("emptyGraph", std::make_unique<metacg::Callgraph>());
  cm.removeAllEstimatorPhases();
  cm.setConfig(&cfg);
  cm.setNoOutput();

  // setup graph
  // ===========
  // main node
  auto mainNode = mcgm.getCallgraph()->getOrInsertNode("main");
  attachAllMetaDataToGraph(mcgm.getCallgraph());
  mainNode->get<pira::PiraOneData>()->setComesFromCube();
  mainNode->get<pira::BaseProfileData>()->setCallData(mainNode, 1, 10.0, 10.0, 0, 0);

  auto child1 = mcgm.getCallgraph()->getOrInsertNode("child1");
  mcgm.getCallgraph()->addEdge(mainNode, child1);

  auto child2 = mcgm.getCallgraph()->getOrInsertNode("child2");
  attachAllMetaDataToGraph(mcgm.getCallgraph());
  child2->get<LoadImbalance::LIMetaData>()->flag(LoadImbalance::FlagType::Irrelevant);
  mcgm.getCallgraph()->addEdge(mainNode, child2);

  auto grandchild = mcgm.getCallgraph()->getOrInsertNode("grandchild");
  mcgm.getCallgraph()->addEdge(child1, grandchild);
  mcgm.getCallgraph()->addEdge(child2, grandchild);
  attachAllMetaDataToGraph(mcgm.getCallgraph());
  for (const auto &elem : mcgm.getCallgraph()->getNodes()) {
    const auto &n = elem.second.get();
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
  LoadImbalance::LIEstimatorPhase lie(std::move(liConfig), cm.getCallgraph());
  cm.registerEstimatorPhase(&lie);
  cm.applyRegisteredPhases();

  auto graph = cm.getCallgraph(&cm);

  ASSERT_EQ(graph->getMain(), mainNode);
  ASSERT_EQ(pgis::isInstrumented(graph->getNode("main")), true);
  ASSERT_EQ(pgis::isInstrumented(graph->getNode("child1")), true);
  ASSERT_EQ(pgis::isInstrumented(graph->getNode("child2")), false);
  ASSERT_EQ(pgis::isInstrumented(graph->getNode("grandchild")), true);
}

TEST_F(LIEstimatorPhaseTest, MajorParentSteps) {
  Config cfg;
  auto &cm = metacg::pgis::PiraMCGProcessor::get();
  auto &mcgm = metacg::graph::MCGManager::get();
  mcgm.resetManager();
  mcgm.addToManagedGraphs("emptyGraph", std::make_unique<metacg::Callgraph>());
  cm.removeAllEstimatorPhases();
  cm.setConfig(&cfg);
  cm.setNoOutput();

  // setup graph
  // ===========
  // main node
  auto mainNode = mcgm.getCallgraph()->getOrInsertNode("main");
  attachAllMetaDataToGraph(mcgm.getCallgraph());
  mainNode->get<pira::PiraOneData>()->setComesFromCube();
  mainNode->get<pira::BaseProfileData>()->setCallData(mainNode, 1, 10.0, 10.0, 0, 0);

  auto child1 = mcgm.getCallgraph()->getOrInsertNode("child1");
  attachAllMetaDataToGraph(mcgm.getCallgraph());
  child1->get<LoadImbalance::LIMetaData>()->flag(LoadImbalance::FlagType::Visited);
  mcgm.getCallgraph()->addEdge(mainNode, child1);

  auto child2 = mcgm.getCallgraph()->getOrInsertNode("child2");
  attachAllMetaDataToGraph(mcgm.getCallgraph());
  child2->get<LoadImbalance::LIMetaData>()->flag(LoadImbalance::FlagType::Visited);
  // child1->addChildNode(child2);  // TODO: Necessary? Remove me!
  mcgm.getCallgraph()->addEdge(child1, child2);

  auto child3 = mcgm.getCallgraph()->getOrInsertNode("child3");
  attachAllMetaDataToGraph(mcgm.getCallgraph());
  child3->get<LoadImbalance::LIMetaData>()->flag(LoadImbalance::FlagType::Visited);
  mcgm.getCallgraph()->addEdge(child2, child3);

  child3->get<pira::PiraOneData>()->setComesFromCube();
  child3->get<pira::BaseProfileData>()->setCallData(mainNode, 1, 10.0, 1.0, 0, 0);
  child3->get<pira::BaseProfileData>()->setCallData(mainNode, 1, 10.0, 100.0, 0, 1);

  cm.setCG(mcgm.getCallgraph());
  // apply estimator phases
  auto liConfig = std::make_unique<LoadImbalance::LIConfig>(LoadImbalance::LIConfig{
      LoadImbalance::MetricType::Efficiency, 1.2, 0.1, LoadImbalance::ContextStrategy::MajorParentSteps, 1,
      LoadImbalance::ChildRelevanceStrategy::ConstantThreshold, 5, 0.0});
  LoadImbalance::LIEstimatorPhase lie(std::move(liConfig), cm.getCallgraph());
  cm.registerEstimatorPhase(&lie);
  cm.applyRegisteredPhases();

  auto graph = cm.getCallgraph(&cm);

  ASSERT_EQ(graph->getMain(), mainNode);
  ASSERT_EQ(pgis::isInstrumented(graph->getNode("main")), true);
  ASSERT_EQ(pgis::isInstrumented(graph->getNode("child1")), false);
  ASSERT_EQ(pgis::isInstrumented(graph->getNode("child2")), true);
  ASSERT_EQ(pgis::isInstrumented(graph->getNode("child3")), true);
}

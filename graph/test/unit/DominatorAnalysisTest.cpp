/**
 * File: DominatorAnalysisTest.cpp
 * License: Part of the metacg project. Licensed under BSD 3 clause license. See LICENSE.txt file at
 * https://github.com/tudasc/metacg/LICENSE.txt
 */

#include "gtest/gtest.h"

#include "DominatorAnalysis.h"
#include "metacg/MCGManager.h"

namespace {
using namespace metacg::analysis;

static const char* const mainS = "main";
static const char* const C1 = "child1";
static const char* const C2 = "child2";
static const char* const C3 = "child3";
static const char* const C4 = "child4";
static const char* const C5 = "child5";
static const char* const C6 = "child6";
static const char* const EXIT = "exit";

class DominatorAnalysisTest : public ::testing::Test {
 protected:
  void SetUp() override {
    metacg::loggerutil::getLogger();
    auto& mcgm = metacg::graph::MCGManager::get();
    mcgm.resetManager();
    mcgm.addToManagedGraphs("testgraph", std::make_unique<metacg::Callgraph>());
  }

  metacg::Callgraph* getGraph() {
    auto& mcgm = metacg::graph::MCGManager::get();
    return mcgm.getCallgraph();
  }
  void fillGraph(metacg::Callgraph* graph) {
    graph->insert(mainS);
    graph->insert(C1);
    graph->insert(C2);
    graph->insert(C3);
    graph->insert(C4);
    graph->insert(C5);
    graph->insert(C6);
    graph->insert(EXIT);
  }
};

TEST_F(DominatorAnalysisTest, Dom_SingleNode) {
  auto cg = getGraph();
  ASSERT_TRUE(cg != nullptr);
  fillGraph(cg);

  auto main = cg->getFirstNode(mainS);

  auto doms = computeDominators<metacg::CgNode, metacg::Callgraph, metacg::analysis::TraverseDir::Forward>(*cg, *main);

  auto& doms_main = doms[main].Doms;
  ASSERT_TRUE(doms_main.size() == 1);
  ASSERT_TRUE(doms_main.count(main) == 1);
}
TEST_F(DominatorAnalysisTest, Dom_LinearCallChain) {
  auto cg = getGraph();
  ASSERT_TRUE(cg != nullptr);
  fillGraph(cg);
  ASSERT_TRUE(cg->addEdge(mainS, C1));
  ASSERT_TRUE(cg->addEdge(C1, C2));
  ASSERT_TRUE(cg->addEdge(C2, EXIT));

  auto doms = computeDominators<metacg::CgNode, metacg::Callgraph, metacg::analysis::TraverseDir::Forward>(
      *cg, *cg->getFirstNode(mainS));

  auto main = cg->getFirstNode(mainS);
  auto node1 = cg->getFirstNode(C1);
  auto node2 = cg->getFirstNode(C2);
  auto exit = cg->getFirstNode(EXIT);

  auto& doms_main = doms[main].Doms;
  ASSERT_TRUE(doms_main.size() == 1);
  ASSERT_TRUE(doms_main.count(main) == 1);

  auto& doms_node1 = doms[node1].Doms;
  ASSERT_TRUE(doms_node1.size() == 2);
  ASSERT_TRUE(doms_node1.count(main) == 1);
  ASSERT_TRUE(doms_node1.count(node1) == 1);

  auto& doms_node2 = doms[node2].Doms;
  ASSERT_TRUE(doms_node2.size() == 3);
  ASSERT_TRUE(doms_node2.count(main) == 1);
  ASSERT_TRUE(doms_node2.count(node1) == 1);
  ASSERT_TRUE(doms_node2.count(node2) == 1);

  auto& doms_exit = doms[exit].Doms;
  ASSERT_TRUE(doms_exit.size() == 4);
  ASSERT_TRUE(doms_exit.count(main) == 1);
  ASSERT_TRUE(doms_exit.count(node1) == 1);
  ASSERT_TRUE(doms_exit.count(node2) == 1);
  ASSERT_TRUE(doms_exit.count(exit) == 1);
}

TEST_F(DominatorAnalysisTest, Dom_BranchAndJoin) {
  auto cg = getGraph();
  ASSERT_TRUE(cg != nullptr);
  fillGraph(cg);
  ASSERT_TRUE(cg->addEdge(mainS, C1));
  ASSERT_TRUE(cg->addEdge(mainS, C3));
  ASSERT_TRUE(cg->addEdge(C1, C2));
  ASSERT_TRUE(cg->addEdge(C2, EXIT));
  ASSERT_TRUE(cg->addEdge(C3, C2));

  auto doms = computeDominators<metacg::CgNode, metacg::Callgraph, metacg::analysis::TraverseDir::Forward>(
      *cg, *cg->getFirstNode(mainS));

  auto main = cg->getFirstNode(mainS);
  auto node1 = cg->getFirstNode(C1);
  auto node2 = cg->getFirstNode(C2);
  auto node3 = cg->getFirstNode(C3);
  auto exit = cg->getFirstNode(EXIT);

  auto& doms_main = doms[main].Doms;
  ASSERT_TRUE(doms_main.size() == 1);
  ASSERT_TRUE(doms_main.count(main) == 1);

  auto& doms_node1 = doms[node1].Doms;
  ASSERT_TRUE(doms_node1.size() == 2);
  ASSERT_TRUE(doms_node1.count(main) == 1);
  ASSERT_TRUE(doms_node1.count(node1) == 1);

  auto& doms_node2 = doms[node2].Doms;
  ASSERT_TRUE(doms_node2.size() == 2);
  ASSERT_TRUE(doms_node2.count(main) == 1);
  ASSERT_FALSE(doms_node2.count(node1) == 1);
  ASSERT_FALSE(doms_node2.count(node3) == 1);
  ASSERT_TRUE(doms_node2.count(node2) == 1);

  auto& doms_node3 = doms[node3].Doms;
  ASSERT_TRUE(doms_node3.size() == 2);
  ASSERT_TRUE(doms_node3.count(main) == 1);
  ASSERT_TRUE(doms_node3.count(node3) == 1);

  auto& doms_exit = doms[exit].Doms;
  ASSERT_TRUE(doms_exit.size() == 3);
  ASSERT_TRUE(doms_exit.count(main) == 1);
  ASSERT_FALSE(doms_exit.count(node1) == 1);
  ASSERT_TRUE(doms_exit.count(node2) == 1);
  ASSERT_FALSE(doms_exit.count(node3) == 1);
  ASSERT_TRUE(doms_exit.count(exit) == 1);
}

TEST_F(DominatorAnalysisTest, Dom_UnreachableNodes) {
  auto cg = getGraph();
  ASSERT_TRUE(cg != nullptr);
  fillGraph(cg);
  ASSERT_TRUE(cg->addEdge(mainS, C1));
  ASSERT_TRUE(cg->addEdge(C2, C3));

  auto doms = computeDominators<metacg::CgNode, metacg::Callgraph, metacg::analysis::TraverseDir::Forward>(
      *cg, *cg->getFirstNode(mainS));

  ASSERT_TRUE(doms.find(cg->getFirstNode(C2)) == doms.end());
}

TEST_F(DominatorAnalysisTest, Dom_Recursion) {
  auto cg = getGraph();
  ASSERT_TRUE(cg != nullptr);
  fillGraph(cg);
  ASSERT_TRUE(cg->addEdge(mainS, C1));
  ASSERT_TRUE(cg->addEdge(C1, C2));
  ASSERT_TRUE(cg->addEdge(C2, C1));
  ASSERT_TRUE(cg->addEdge(C2, EXIT));

  auto doms = computeDominators<metacg::CgNode, metacg::Callgraph, metacg::analysis::TraverseDir::Forward>(
      *cg, *cg->getFirstNode(mainS));

  auto main = cg->getFirstNode(mainS);
  auto node1 = cg->getFirstNode(C1);
  auto node2 = cg->getFirstNode(C2);
  auto exit = cg->getFirstNode(EXIT);

  auto& doms_main = doms[main].Doms;
  ASSERT_TRUE(doms_main.size() == 1);
  ASSERT_TRUE(doms_main.count(main) == 1);

  auto& doms_node1 = doms[node1].Doms;
  ASSERT_TRUE(doms_node1.size() == 2);
  ASSERT_TRUE(doms_node1.count(main) == 1);
  ASSERT_TRUE(doms_node1.count(node1) == 1);

  auto& doms_node2 = doms[node2].Doms;
  ASSERT_TRUE(doms_node2.size() == 3);
  ASSERT_TRUE(doms_node2.count(main) == 1);
  ASSERT_TRUE(doms_node2.count(node1) == 1);
  ASSERT_TRUE(doms_node2.count(node2) == 1);

  auto& doms_exit = doms[exit].Doms;
  ASSERT_TRUE(doms_exit.size() == 4);
  ASSERT_TRUE(doms_exit.count(main) == 1);
  ASSERT_TRUE(doms_exit.count(node1) == 1);
  ASSERT_TRUE(doms_exit.count(node2) == 1);
  ASSERT_TRUE(doms_exit.count(exit) == 1);
}

TEST_F(DominatorAnalysisTest, Dom_MultiNodeCycle) {
  auto cg = getGraph();
  ASSERT_TRUE(cg != nullptr);
  fillGraph(cg);

  ASSERT_TRUE(cg->addEdge(mainS, C1));
  ASSERT_TRUE(cg->addEdge(C1, C2));
  ASSERT_TRUE(cg->addEdge(C2, C3));
  ASSERT_TRUE(cg->addEdge(C3, C1));
  ASSERT_TRUE(cg->addEdge(C3, EXIT));

  auto doms = computeDominators<metacg::CgNode, metacg::Callgraph, metacg::analysis::TraverseDir::Forward>(
      *cg, *cg->getFirstNode(mainS));

  auto main = cg->getFirstNode(mainS);
  auto node1 = cg->getFirstNode(C1);
  auto node2 = cg->getFirstNode(C2);
  auto node3 = cg->getFirstNode(C3);
  auto exit = cg->getFirstNode(EXIT);

  ASSERT_TRUE(doms[main].Doms.count(main) == 1);

  ASSERT_TRUE(doms[node1].Doms.count(main) == 1);
  ASSERT_TRUE(doms[node1].Doms.count(node1) == 1);

  ASSERT_TRUE(doms[node2].Doms.count(main) == 1);
  ASSERT_TRUE(doms[node2].Doms.count(node1) == 1);
  ASSERT_TRUE(doms[node2].Doms.count(node2) == 1);

  ASSERT_TRUE(doms[node3].Doms.count(main) == 1);
  ASSERT_TRUE(doms[node3].Doms.count(node1) == 1);
  ASSERT_TRUE(doms[node3].Doms.count(node2) == 1);
  ASSERT_TRUE(doms[node3].Doms.count(node3) == 1);

  ASSERT_TRUE(doms[exit].Doms.count(main) == 1);
  ASSERT_TRUE(doms[exit].Doms.count(node1) == 1);
  ASSERT_TRUE(doms[exit].Doms.count(node2) == 1);
  ASSERT_TRUE(doms[exit].Doms.count(node3) == 1);
  ASSERT_TRUE(doms[exit].Doms.count(exit) == 1);
}

TEST_F(DominatorAnalysisTest, PostDom_SingleNode) {
  auto cg = getGraph();
  ASSERT_TRUE(cg != nullptr);
  fillGraph(cg);

  auto exitNode = cg->getFirstNode(EXIT);

  auto pdoms = computeDominators<metacg::CgNode, metacg::Callgraph, metacg::analysis::TraverseDir::Backward>(*cg, *exitNode);

  auto& pdoms_exit = pdoms[exitNode].Doms;
  ASSERT_TRUE(pdoms_exit.size() == 1);
  ASSERT_TRUE(pdoms_exit.count(exitNode));
}

TEST_F(DominatorAnalysisTest, PostDom_LinearCallChain) {
  auto cg = getGraph();
  ASSERT_TRUE(cg != nullptr);
  fillGraph(cg);
  ASSERT_TRUE(cg->addEdge(mainS, C1));
  ASSERT_TRUE(cg->addEdge(C1, C2));
  ASSERT_TRUE(cg->addEdge(C2, EXIT));

  auto pdoms = computeDominators<metacg::CgNode, metacg::Callgraph, metacg::analysis::TraverseDir::Backward>(
      *cg, *cg->getFirstNode(EXIT));

  auto main = cg->getFirstNode(mainS);
  auto node1 = cg->getFirstNode(C1);
  auto node2 = cg->getFirstNode(C2);
  auto exit = cg->getFirstNode(EXIT);

  ASSERT_TRUE(pdoms[exit].Doms.count(exit) == 1);

  ASSERT_TRUE(pdoms[node2].Doms.count(node2) == 1);
  ASSERT_TRUE(pdoms[node2].Doms.count(exit) == 1);

  ASSERT_TRUE(pdoms[node1].Doms.count(node1) == 1);
  ASSERT_TRUE(pdoms[node1].Doms.count(node2) == 1);
  ASSERT_TRUE(pdoms[node1].Doms.count(exit) == 1);

  ASSERT_TRUE(pdoms[main].Doms.count(main) == 1);
  ASSERT_TRUE(pdoms[main].Doms.count(node1) == 1);
  ASSERT_TRUE(pdoms[main].Doms.count(node2) == 1);
  ASSERT_TRUE(pdoms[main].Doms.count(exit) == 1);
}

TEST_F(DominatorAnalysisTest, PostDom_BranchAndJoin) {
  auto cg = getGraph();
  ASSERT_TRUE(cg != nullptr);
  fillGraph(cg);
  ASSERT_TRUE(cg->addEdge(mainS, C1));
  ASSERT_TRUE(cg->addEdge(mainS, C3));
  ASSERT_TRUE(cg->addEdge(C1, C2));
  ASSERT_TRUE(cg->addEdge(C2, EXIT));
  ASSERT_TRUE(cg->addEdge(C3, C2));

  auto pdoms = computeDominators<metacg::CgNode, metacg::Callgraph, metacg::analysis::TraverseDir::Backward>(
      *cg, *cg->getFirstNode(EXIT));

  auto main = cg->getFirstNode(mainS);
  auto node1 = cg->getFirstNode(C1);
  auto node2 = cg->getFirstNode(C2);
  auto node3 = cg->getFirstNode(C3);
  auto exit = cg->getFirstNode(EXIT);

  ASSERT_TRUE(pdoms[exit].Doms.count(exit) == 1);

  ASSERT_TRUE(pdoms[node2].Doms.count(node2) == 1);
  ASSERT_TRUE(pdoms[node2].Doms.count(exit) == 1);

  ASSERT_TRUE(pdoms[node1].Doms.count(node1) == 1);
  ASSERT_TRUE(pdoms[node1].Doms.count(node2) == 1);
  ASSERT_TRUE(pdoms[node1].Doms.count(exit) == 1);

  ASSERT_TRUE(pdoms[node3].Doms.count(node3) == 1);
  ASSERT_TRUE(pdoms[node3].Doms.count(node2) == 1);
  ASSERT_TRUE(pdoms[node3].Doms.count(exit) == 1);

  ASSERT_TRUE(pdoms[main].Doms.count(main) == 1);
  ASSERT_FALSE(pdoms[main].Doms.count(node1) == 1);
  ASSERT_TRUE(pdoms[main].Doms.count(node2) == 1);
  ASSERT_FALSE(pdoms[main].Doms.count(node3) == 1);
  ASSERT_TRUE(pdoms[main].Doms.count(exit) == 1);
}

TEST_F(DominatorAnalysisTest, PostDom_UnreachableNodes) {
  auto cg = getGraph();
  ASSERT_TRUE(cg != nullptr);
  fillGraph(cg);
  ASSERT_TRUE(cg->addEdge(mainS, C1));
  ASSERT_TRUE(cg->addEdge(C2, C3));

  auto pdoms = computeDominators<metacg::CgNode, metacg::Callgraph, metacg::analysis::TraverseDir::Backward>(
      *cg, *cg->getFirstNode(EXIT));

  ASSERT_TRUE(pdoms.find(cg->getFirstNode(C2)) == pdoms.end());
}

TEST_F(DominatorAnalysisTest, PostDom_Recursion) {
  auto cg = getGraph();
  ASSERT_TRUE(cg != nullptr);
  fillGraph(cg);
  ASSERT_TRUE(cg->addEdge(mainS, C1));
  ASSERT_TRUE(cg->addEdge(C1, C2));
  ASSERT_TRUE(cg->addEdge(C2, C1));
  ASSERT_TRUE(cg->addEdge(C2, EXIT));

  auto pdoms = computeDominators<metacg::CgNode, metacg::Callgraph, metacg::analysis::TraverseDir::Backward>(
      *cg, *cg->getFirstNode(EXIT));

  auto main = cg->getFirstNode(mainS);
  auto node1 = cg->getFirstNode(C1);
  auto node2 = cg->getFirstNode(C2);
  auto exit = cg->getFirstNode(EXIT);

  ASSERT_TRUE(pdoms[node2].Doms.count(node2) == 1);
  ASSERT_TRUE(pdoms[node2].Doms.count(exit) == 1);

  ASSERT_TRUE(pdoms[node1].Doms.count(node1) == 1);
  ASSERT_TRUE(pdoms[node1].Doms.count(node2) == 1);
  ASSERT_TRUE(pdoms[node1].Doms.count(exit) == 1);

  ASSERT_TRUE(pdoms[main].Doms.count(main) == 1);
  ASSERT_TRUE(pdoms[main].Doms.count(node1) == 1);
  ASSERT_TRUE(pdoms[main].Doms.count(node2) == 1);
  ASSERT_TRUE(pdoms[main].Doms.count(exit) == 1);

  ASSERT_TRUE(pdoms[exit].Doms.count(exit));
}

TEST_F(DominatorAnalysisTest, PostDom_MultiNodeCycle) {
  auto cg = getGraph();
  ASSERT_TRUE(cg != nullptr);
  fillGraph(cg);

  ASSERT_TRUE(cg->addEdge(mainS, C1));
  ASSERT_TRUE(cg->addEdge(C1, C2));
  ASSERT_TRUE(cg->addEdge(C2, C3));
  ASSERT_TRUE(cg->addEdge(C3, C1));
  ASSERT_TRUE(cg->addEdge(C3, EXIT));

  auto pdoms = computeDominators<metacg::CgNode, metacg::Callgraph, metacg::analysis::TraverseDir::Backward>(
      *cg, *cg->getFirstNode(EXIT));

  auto main = cg->getFirstNode(mainS);
  auto node1 = cg->getFirstNode(C1);
  auto node2 = cg->getFirstNode(C2);
  auto node3 = cg->getFirstNode(C3);
  auto exit = cg->getFirstNode(EXIT);

  ASSERT_TRUE(pdoms[exit].Doms.count(exit) == 1);

  ASSERT_TRUE(pdoms[node3].Doms.count(node3) == 1);
  ASSERT_TRUE(pdoms[node3].Doms.count(exit) == 1);

  ASSERT_TRUE(pdoms[node2].Doms.count(node2) == 1);
  ASSERT_TRUE(pdoms[node2].Doms.count(node3) == 1);
  ASSERT_TRUE(pdoms[node2].Doms.count(exit) == 1);

  ASSERT_TRUE(pdoms[node1].Doms.count(node1) == 1);
  ASSERT_TRUE(pdoms[node1].Doms.count(node2) == 1);
  ASSERT_TRUE(pdoms[node1].Doms.count(node3) == 1);
  ASSERT_TRUE(pdoms[node1].Doms.count(exit) == 1);

  ASSERT_TRUE(pdoms[main].Doms.count(main) == 1);
  ASSERT_TRUE(pdoms[main].Doms.count(node1) == 1);
  ASSERT_TRUE(pdoms[main].Doms.count(node2) == 1);
  ASSERT_TRUE(pdoms[main].Doms.count(node3) == 1);
  ASSERT_TRUE(pdoms[main].Doms.count(exit) == 1);
}

}  // namespace

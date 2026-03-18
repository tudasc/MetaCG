/**
 * File: CallgraphTest.cpp
 * License: Part of the MetaCG project. Licensed under BSD 3 clause license. See LICENSE.txt file at
 * https://github.com/tudasc/metacg/LICENSE.txt
 */

#include "Callgraph.h"
#include "CgNode.h"
#include "metadata/OverrideMD.h"
#include "gtest/gtest.h"

namespace {
TEST(Callgraph, EmpytCG) {
  metacg::Callgraph cg;

  EXPECT_EQ(cg.getMain(), nullptr);
  EXPECT_EQ(cg.hasNode("foo"), false);
  EXPECT_EQ(cg.hasDuplicateNames(), false);
  EXPECT_EQ(cg.getFirstNode("foo"), nullptr);
  EXPECT_EQ(cg.size(), 0);
  EXPECT_EQ(cg.getNodeCount(), 0);

  // Query some random "node ID"
  {
    auto callees = cg.getCallees(0);
    EXPECT_EQ(callees.size(), 0);
  }
  {
    auto callers = cg.getCallers(0);
    EXPECT_EQ(callers.size(), 0);
  }
}

TEST(Callgraph, getOrInsertOneNodeNotMain) {
  metacg::Callgraph cg;

  EXPECT_EQ(cg.size(), 0);
  EXPECT_EQ(cg.getNodeCount(), 0);

  std::string n1Name("n1");
  cg.getOrInsertNode(n1Name);

  EXPECT_EQ(cg.size(), 1);
  EXPECT_EQ(cg.getNodeCount(), 1);

  // We did not insert a main node
  EXPECT_EQ(cg.getMain(), nullptr);

  auto n1Node = cg.getFirstNode(n1Name);
  EXPECT_EQ(n1Node->getFunctionName(), n1Name);
}

TEST(Callgraph, getOrInsertMultiIdentNoOrigin) {
  metacg::Callgraph cg;

  std::string n1Name("foo");
  cg.getOrInsertNode(n1Name);
  // This does not insert a new node and does not change the existing one
  cg.getOrInsertNode(n1Name, {}, true);
  cg.getOrInsertNode(n1Name, {}, true, true);
  EXPECT_EQ(cg.getNodeCount(), 1);

  auto n1Node = cg.getFirstNode(n1Name);
  EXPECT_EQ(n1Node->has<metacg::OverrideMD>(), false);
}

TEST(Callgraph, getOrInsertMultiIdentDiffOrigin) {
  metacg::Callgraph cg;

  std::string n1Name("foo");
  std::string n1OrigA("Orig::A");
  std::string n1OrigB("Orig::B");
  cg.getOrInsertNode(n1Name, n1OrigA);
  // getOrInsertNode returns the first node if already exists with that name
  cg.getOrInsertNode(n1Name, n1OrigB, true);
  cg.getOrInsertNode(n1Name, n1OrigB, true, true);
  EXPECT_EQ(cg.getNodeCount(), 1);

  // getOrInsertNode returned an existing node
  EXPECT_FALSE(cg.hasDuplicateNames());

  auto n1Node = cg.getFirstNode(n1Name);
  EXPECT_EQ(n1Node->has<metacg::OverrideMD>(), false);
  EXPECT_EQ(cg.getMain(), nullptr);
}

TEST(Callgraph, insertGetOrInsertMultiIdentNoOrigin) {
  metacg::Callgraph cg;

  std::string n1Name("foo");
  cg.insert(n1Name);
  // This does not insert a new node and does not change the existing one
  cg.getOrInsertNode(n1Name, {}, true);
  cg.getOrInsertNode(n1Name, {}, true, true);
  EXPECT_EQ(cg.getNodeCount(), 1);

  // getOrInsertNode returned an existing node
  EXPECT_FALSE(cg.hasDuplicateNames());

  auto n1Node = cg.getFirstNode(n1Name);
  EXPECT_EQ(n1Node->has<metacg::OverrideMD>(), false);
  EXPECT_EQ(cg.getMain(), nullptr);
}

TEST(Callgraph, insertGetOrInsertMultiIdentDiffOrigin) {
  metacg::Callgraph cg;

  std::string n1Name("foo");
  std::string n1OrigA("Orig::A");
  std::string n1OrigB("Orig::B");
  // insert always inserts a new node
  cg.insert(n1Name, n1OrigA);
  cg.insert(n1Name, n1OrigA, true);
  // getOrInsertNode returns the first node if already exists with that name
  cg.getOrInsertNode(n1Name, n1OrigB, true, true);
  EXPECT_EQ(cg.getNodeCount(), 2);

  // insert does add a new node with the same name
  EXPECT_TRUE(cg.hasDuplicateNames());

  auto n1Node = cg.getFirstNode(n1Name);
  EXPECT_EQ(n1Node->has<metacg::OverrideMD>(), false);
  EXPECT_EQ(cg.getMain(), nullptr);
}

TEST(Callgraph, insertMultiIdentDiffOrigin) {
  metacg::Callgraph cg;

  std::string n1Name("foo");
  std::string n1OrigA("Orig::A");
  std::string n1OrigB("Orig::B");
  // insert always inserts a new node
  cg.insert(n1Name, n1OrigA);
  cg.insert(n1Name, n1OrigB);
  EXPECT_EQ(cg.getNodeCount(), 2);
  EXPECT_EQ(cg.countNodes(n1Name), 2);

  // insert does add a new node with the same name
  EXPECT_TRUE(cg.hasDuplicateNames());

  auto n1Node = cg.getFirstNode(n1Name);
  EXPECT_EQ(n1Node->getOrigin(), n1OrigA);
}

TEST(Callgraph, singleEdge) {
  metacg::Callgraph cg;

  std::string n1Name("foo");
  std::string n1OrigA("Orig::A");
  cg.insert(n1Name);

  // No edge exists by default
  EXPECT_FALSE(cg.existsEdge(cg.getSingleNode(n1Name), cg.getSingleNode(n1Name)));

  // Insert single edge (by Node&)
  EXPECT_TRUE(cg.addEdge(cg.getSingleNode(n1Name), cg.getSingleNode(n1Name)));
  EXPECT_TRUE(cg.existsEdge(cg.getSingleNode(n1Name), cg.getSingleNode(n1Name)));
  EXPECT_FALSE(cg.addEdge(cg.getSingleNode(n1Name), cg.getSingleNode(n1Name)));

  // Remove single edge (by Node&)
  EXPECT_TRUE(cg.removeEdge(cg.getSingleNode(n1Name), cg.getSingleNode(n1Name)));
  EXPECT_FALSE(cg.existsEdge(cg.getSingleNode(n1Name), cg.getSingleNode(n1Name)));
  EXPECT_FALSE(cg.removeEdge(cg.getSingleNode(n1Name), cg.getSingleNode(n1Name)));

  // Insert single edge (by name)
  EXPECT_TRUE(cg.addEdge(n1Name, n1Name));
  EXPECT_TRUE(cg.existsEdge(cg.getSingleNode(n1Name), cg.getSingleNode(n1Name)));
  EXPECT_FALSE(cg.addEdge(n1Name, n1Name));

  // Remove single edge (by name)
  EXPECT_TRUE(cg.removeEdge(n1Name, n1Name));
  EXPECT_FALSE(cg.existsEdge(cg.getSingleNode(n1Name), cg.getSingleNode(n1Name)));
  EXPECT_FALSE(cg.removeEdge(n1Name, n1Name));
}

TEST(Callgraph, singleEdgeMultiNode) {
  metacg::Callgraph cg;

  std::string n1Name("foo");
  std::string n1OrigA("Orig::A");
  std::string n1OrigB("Orig::B");
  // insert always inserts a new node
  auto& n1a = cg.insert(n1Name, n1OrigA);
  auto& n1b = cg.insert(n1Name, n1OrigB);

  // Graph holds two nodes with the same name, no insertion
  EXPECT_FALSE(cg.addEdge(n1Name, n1Name));

  auto nodes = cg.getNodes(n1Name);
  // For individual nodes with name n1Name, add an edge, and remove it again
  // to see if the behavior matches
  for (const auto n : nodes) {
    EXPECT_TRUE(cg.addEdge(n, n));
    EXPECT_FALSE(cg.addEdge(n, n));

    EXPECT_TRUE(cg.existsEdge(n, n));

    EXPECT_TRUE(cg.removeEdge(n, n));
    EXPECT_FALSE(cg.removeEdge(n, n));

    EXPECT_FALSE(cg.existsEdge(n, n));
  }

  EXPECT_TRUE(cg.addEdge(n1a, n1a));
  EXPECT_FALSE(cg.addEdge(n1a, n1a));
  EXPECT_TRUE(cg.existsAnyEdge(n1Name, n1Name));
  {
    // Only that particular edge exists.
    auto callees = cg.getCallees(n1a);
    EXPECT_EQ(&n1a, *(callees.begin()));
  }
  EXPECT_FALSE(cg.removeEdge(n1Name, n1Name));
  EXPECT_TRUE(cg.removeEdge(n1a, n1a));
  EXPECT_FALSE(cg.removeEdge(n1a, n1a));
}
}  // namespace

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

  auto n1Node = cg.getFirstNode(n1Name);
  EXPECT_EQ(n1Node->has<metacg::OverrideMD>(), false);

}

TEST(Callgraph, insertGetOrInsertMultiIdentNoOrigin) {
  metacg::Callgraph cg;

  std::string n1Name("foo");
  cg.insert(n1Name);
  // This does not insert a new node and does not change the existing one
  cg.getOrInsertNode(n1Name, {}, true);
  cg.getOrInsertNode(n1Name, {}, true, true);
  EXPECT_EQ(cg.getNodeCount(), 1);

  auto n1Node = cg.getFirstNode(n1Name);
  EXPECT_EQ(n1Node->has<metacg::OverrideMD>(), false);

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

  auto n1Node = cg.getFirstNode(n1Name);
  EXPECT_EQ(n1Node->has<metacg::OverrideMD>(), false);
}
} // anonymous

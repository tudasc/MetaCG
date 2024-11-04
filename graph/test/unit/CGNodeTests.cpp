/**
 * File: CGNodeTests.cpp
 * License: Part of the MetaCG project. Licensed under BSD 3 clause license. See LICENSE.txt file at
 * https://github.com/tudasc/metacg/LICENSE.txt
 */

#include "CgNode.h"
#include "gtest/gtest.h"

TEST(CgNode, CreateNodeOnlyName) {
  auto n = new metacg::CgNode("foo");
  EXPECT_NE(n, nullptr);
  EXPECT_EQ(n->getFunctionName(), "foo");

  delete n;
}

TEST(CgNode, CreateNodeNameAndOrigin) {
  auto n = new metacg::CgNode("foo", "origin:21");

  EXPECT_NE(n, nullptr);
  EXPECT_EQ(n->getFunctionName(), "foo");
  EXPECT_EQ((n->getOrigin()), "origin:21");

  delete n;
}

TEST(CgNode, NodesSame) {
  auto n = new metacg::CgNode("foo");
  auto nn = new metacg::CgNode("foo");

  EXPECT_NE(n, nullptr);
  EXPECT_NE(nn, nullptr);
  EXPECT_TRUE(n->isSameFunctionName(*nn));
  EXPECT_TRUE(nn->isSameFunctionName(*n));

  delete n;
  delete nn;
}

TEST(CgNode, NodesNotSameName) {
  auto n = new metacg::CgNode("foo");
  auto nn = new metacg::CgNode("bar");

  EXPECT_NE(n, nullptr);
  EXPECT_NE(nn, nullptr);
  EXPECT_FALSE(n->isSameFunctionName(*nn));
  EXPECT_FALSE(nn->isSameFunctionName(*n));

  delete n;
  delete nn;
}

TEST(CgNode, NodesNameAndOriginSame) {
  auto n = new metacg::CgNode("foo", "origin:a");
  auto nn = new metacg::CgNode("foo", "origin:a");

  EXPECT_NE(n, nullptr);
  EXPECT_NE(nn, nullptr);
  EXPECT_TRUE(n->isSameFunctionName(*nn));
  EXPECT_TRUE(nn->isSameFunctionName(*n));

  delete n;
  delete nn;
}

TEST(CgNode, NodesNameAndOriginNotSame) {
  auto n = new metacg::CgNode("foo", "origin:a");
  auto nn = new metacg::CgNode("foo", "origin:b");

  EXPECT_NE(n, nullptr);
  EXPECT_NE(nn, nullptr);
  EXPECT_TRUE(n->isSameFunctionName(*nn));
  EXPECT_TRUE(nn->isSameFunctionName(*n));

  EXPECT_FALSE(n->isSameFunction(*nn));
  EXPECT_FALSE(nn->isSameFunction(*n));

  delete n;
  delete nn;
}

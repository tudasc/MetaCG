/**
 * File: CGNodeTests.cpp
 * License: Part of the MetaCG project. Licensed under BSD 3 clause license. See LICENSE.txt file at
 * https://github.com/tudasc/metacg/LICENSE.txt
 */

#include "Callgraph.h"
#include "CgNode.h"
#include "metadata/OverrideMD.h"
#include "gtest/gtest.h"

TEST(CgNode, CreateNodeOnlyName) {
  auto cg = std::make_unique<metacg::Callgraph>();
  auto& n = cg->insert("foo");
  EXPECT_EQ(n.getFunctionName(), "foo");
}

TEST(CgNode, CreateNodeNameAndOrigin) {
  auto cg = std::make_unique<metacg::Callgraph>();
  auto& n = cg->insert("foo", "origin:21");

  EXPECT_EQ(n.getFunctionName(), "foo");
  EXPECT_EQ((n.getOrigin()), "origin:21");
}

TEST(CgNode, NodesSame) {
  auto cg = std::make_unique<metacg::Callgraph>();
  auto& n = cg->insert("foo");
  auto& nn = cg->insert("foo");

  EXPECT_NE(n.getId(), nn.getId());
  EXPECT_TRUE(n.getFunctionName() == nn.getFunctionName());
}

TEST(CgNode, NodesNotSameName) {
  auto cg = std::make_unique<metacg::Callgraph>();
  auto& n = cg->insert("foo");
  auto& nn = cg->insert("bar");

  EXPECT_NE(n.getId(), nn.getId());
  EXPECT_FALSE(n.getFunctionName() == nn.getFunctionName());
}

TEST(CgNode, NodesNameAndOriginSame) {
  auto cg = std::make_unique<metacg::Callgraph>();
  auto& n = cg->insert("foo", "origin:a");
  auto& nn = cg->insert("foo", "origin:a");

  EXPECT_NE(n.getId(), nn.getId());
  EXPECT_TRUE(n.getFunctionName() == nn.getFunctionName());
}

TEST(CgNode, NodesNameAndOriginNotSame) {
  auto cg = std::make_unique<metacg::Callgraph>();
  auto& n = cg->insert("foo", "origin:a");
  auto& nn = cg->insert("foo", "origin:b");

  EXPECT_NE(n.getId(), nn.getId());
  EXPECT_TRUE(n.getFunctionName() == nn.getFunctionName());
  EXPECT_FALSE(n.getOrigin() == nn.getOrigin());
}

TEST(CgNode, AddMD) {
  auto cg = std::make_unique<metacg::Callgraph>();
  auto& n = cg->insert("foo");

  n.addMetaData<metacg::OverrideMD>();

  EXPECT_TRUE(n.has<metacg::OverrideMD>());
  EXPECT_TRUE(n.has(metacg::OverrideMD::key));
  EXPECT_FALSE(n.get<metacg::OverrideMD>() == nullptr);
  EXPECT_FALSE(n.get(metacg::OverrideMD::key) == nullptr);
}

TEST(CgNode, EraseMD) {
  auto cg = std::make_unique<metacg::Callgraph>();
  auto& n = cg->insert("foo");

  n.addMetaData<metacg::OverrideMD>();

  EXPECT_TRUE(n.has<metacg::OverrideMD>());
  EXPECT_TRUE(n.erase<metacg::OverrideMD>());
  EXPECT_FALSE(n.has<metacg::OverrideMD>());
  EXPECT_FALSE(n.erase<metacg::OverrideMD>());
}

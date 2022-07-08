//
// Created by jp on 01.07.22.
//

#include "CgNode.h"
#include "LoggerUtil.h"
#include "gtest/gtest.h"

using namespace metacg;

namespace {
class CgNodeBasics : public ::testing::Test {
 protected:
  void SetUp() override { metacg::loggerutil::getLogger(); }
};

TEST_F(CgNodeBasics, CreateChildParentNodeDefaults) {
  auto n = std::make_shared<CgNode>("parent");
  auto c = std::make_shared<CgNode>("child");
  ASSERT_EQ(0, n->getChildNodes().size());
  ASSERT_EQ(0, c->getParentNodes().size());
  n->addChildNode(c);
  c->addParentNode(n);
  ASSERT_EQ(1, n->getChildNodes().size());
  ASSERT_EQ(1, c->getParentNodes().size());
  ASSERT_TRUE(c->isSameFunction(*(n->getChildNodes().begin())));
  ASSERT_TRUE(n->isSameFunction(*(c->getParentNodes().begin())));
}

TEST_F(CgNodeBasics, CreateChildParentRemove) {
  auto n = std::make_shared<CgNode>("parent");
  auto c = std::make_shared<CgNode>("child");
  n->addChildNode(c);
  c->addParentNode(n);
  n->removeChildNode(c);
  ASSERT_EQ(0, n->getChildNodes().size());
  ASSERT_EQ(1, c->getParentNodes().size());
  c->removeParentNode(n);
  ASSERT_EQ(0, c->getParentNodes().size());
}
}

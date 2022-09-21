/**
 * File: CgNodeTest.cpp
 * License: Part of the metacg project. Licensed under BSD 3 clause license. See LICENSE.txt file at
 * https://github.com/tudasc/metacg/LICENSE.txt
 */

#include "CgNode.h"
#include "MetaData/CgNodeMetaData.h"
#include "LoggerUtil.h"
#include "CgHelper.h"

#include "gtest/gtest.h"

using namespace pira;
using namespace metacg;

namespace {
class CgNodeBasics : public ::testing::Test {
 protected:
  void SetUp() override {
    metacg::loggerutil::getLogger();
  }
  static CgNodePtr getNodeWithMD(const std::string &name) {
    auto n = std::make_shared<CgNode>(name);
    getOrCreateMD<pira::BaseProfileData>(n);
    getOrCreateMD<pira::PiraOneData>(n);
    getOrCreateMD<pira::PiraTwoData>(n);
    return n;
  }
};

TEST_F(CgNodeBasics, CreateNodeDefaults) {
  auto n = getNodeWithMD("foo");
  ASSERT_STREQ("foo", n->getFunctionName().c_str());
  ASSERT_TRUE(n->get<BaseProfileData>());
  ASSERT_EQ(0.0, n->get<BaseProfileData>()->getRuntimeInSeconds());
  ASSERT_EQ(.0, n->get<BaseProfileData>()->getInclusiveRuntimeInSeconds());
  ASSERT_EQ(0, n->get<BaseProfileData>()->getNumberOfCalls());
  ASSERT_TRUE(n->get<PiraOneData>());
  ASSERT_EQ(0, n->get<PiraOneData>()->getNumberOfStatements());
  ASSERT_EQ(false, n->get<PiraOneData>()->inPreviousProfile());
  ASSERT_EQ(0, n->getParentNodes().size());
  ASSERT_EQ(0, n->getChildNodes().size());
  ASSERT_EQ(true, n->isLeafNode());
  ASSERT_EQ(true, n->isRootNode());
}

TEST_F(CgNodeBasics, CreateSingleNodeWithRuntime) {
  auto n = getNodeWithMD("foo");
  ASSERT_TRUE(n->get<BaseProfileData>());
  n->get<BaseProfileData>()->setRuntimeInSeconds(1.23);
  n->get<BaseProfileData>()->setInclusiveRuntimeInSeconds(1.23);
  ASSERT_EQ(1.23, n->get<BaseProfileData>()->getRuntimeInSeconds());
  // This is true for nodes w/o children
  ASSERT_EQ(1.23, n->get<BaseProfileData>()->getInclusiveRuntimeInSeconds());
}

TEST_F(CgNodeBasics, CreateSingleNodeWithStatements) {
  auto n = getNodeWithMD("foo");
  ASSERT_TRUE(n->get<BaseProfileData>());
  ASSERT_TRUE(n->get<PiraOneData>());
  n->get<PiraOneData>()->setNumberOfStatements(42);
  ASSERT_EQ(42, n->get<PiraOneData>()->getNumberOfStatements());
}

TEST_F(CgNodeBasics, CreateChildNodeDefaults) {
  auto n = getNodeWithMD("foo");
  auto c = getNodeWithMD("child");
  ASSERT_TRUE(n->get<BaseProfileData>());
  ASSERT_TRUE(n->get<PiraOneData>());
  ASSERT_TRUE(c->get<BaseProfileData>());
  ASSERT_TRUE(c->get<PiraOneData>());
  ASSERT_EQ(0, n->getChildNodes().size());
  ASSERT_EQ(0, c->getParentNodes().size());
  n->addChildNode(c);
  ASSERT_EQ(1, n->getChildNodes().size());
  ASSERT_EQ(0, c->getParentNodes().size());
  ASSERT_TRUE(c->isSameFunction(*(n->getChildNodes().begin())));
}


TEST_F(CgNodeBasics, CreateChildParentRuntime) {
  auto n = getNodeWithMD("foo");
  auto c = getNodeWithMD("child");
  ASSERT_TRUE(n->get<BaseProfileData>());
  ASSERT_TRUE(n->get<PiraOneData>());
  ASSERT_TRUE(c->get<BaseProfileData>());
  ASSERT_TRUE(c->get<PiraOneData>());
  c->get<PiraOneData>()->setComesFromCube();
  n->get<PiraOneData>()->setComesFromCube();
  n->get<BaseProfileData>()->setRuntimeInSeconds(1.25);
  n->get<BaseProfileData>()->setInclusiveRuntimeInSeconds(1.5);
  c->get<BaseProfileData>()->setRuntimeInSeconds(0.25);
  c->get<BaseProfileData>()->setInclusiveRuntimeInSeconds(0.25);
  n->addChildNode(c);
  c->addParentNode(n);
  ASSERT_EQ(1.25, n->get<BaseProfileData>()->getRuntimeInSeconds());
  ASSERT_EQ(0.25, c->get<BaseProfileData>()->getRuntimeInSeconds());
  ASSERT_EQ(1.5, n->get<BaseProfileData>()->getInclusiveRuntimeInSeconds());
  ASSERT_EQ(true, c->isLeafNode());
  ASSERT_EQ(true, n->isRootNode());
  ASSERT_EQ(false, n->isLeafNode());
  ASSERT_EQ(false, c->isRootNode());
}


}  // namespace

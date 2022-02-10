/**
 * File: CgNodeTest.cpp
 * License: Part of the metacg project. Licensed under BSD 3 clause license. See LICENSE.txt file at https://github.com/tudasc/metacg/LICENSE.txt
 */

#include "../../../graph/include/CgNode.h"
#include "CgHelper.h"

#include "LoggerUtil.h"

#include "gtest/gtest.h"

using namespace pira;
using namespace metacg;

namespace {
class CgNodeBasics : public ::testing::Test {
 protected:
  void SetUp() override { loggerutil::getLogger(); }
};

TEST_F(CgNodeBasics, CreateNodeDefaults) {
  auto n = std::make_shared<CgNode>("foo");
  ASSERT_STREQ("foo", n->getFunctionName().c_str());
  ASSERT_TRUE(n->get<BaseProfileData>());
  ASSERT_EQ(-1, n->getLineNumber());
  ASSERT_EQ(0.0, n->get<BaseProfileData>()->getRuntimeInSeconds());
  ASSERT_EQ(.0, n->get<BaseProfileData>()->getInclusiveRuntimeInSeconds());
  ASSERT_EQ(0, n->get<BaseProfileData>()->getNumberOfCalls());
  ASSERT_TRUE(n->get<PiraOneData>());
  ASSERT_EQ(0, n->get<PiraOneData>()->getNumberOfStatements());
  ASSERT_EQ(false, n->get<PiraOneData>()->inPreviousProfile());
  ASSERT_EQ(false, n->hasUniqueCallPath());
  ASSERT_EQ(0, n->getParentNodes().size());
  ASSERT_EQ(0, n->getChildNodes().size());
  ASSERT_EQ(0, n->getSpantreeParents().size());
  ASSERT_EQ(true, n->isLeafNode());
  ASSERT_EQ(true, n->isRootNode());
}

TEST_F(CgNodeBasics, CreateSingleNodeWithRuntime) {
  auto n = std::make_shared<CgNode>("foo");
  ASSERT_TRUE(n->get<BaseProfileData>());
  n->get<BaseProfileData>()->setRuntimeInSeconds(1.23);
  n->get<BaseProfileData>()->setInclusiveRuntimeInSeconds(1.23);
  ASSERT_EQ(1.23, n->get<BaseProfileData>()->getRuntimeInSeconds());
  // This is true for nodes w/o children
  ASSERT_EQ(1.23, n->get<BaseProfileData>()->getInclusiveRuntimeInSeconds());
}

TEST_F(CgNodeBasics, CreateSingleNodeWithStatements) {
  auto n = std::make_shared<CgNode>("foo");
  ASSERT_TRUE(n->get<BaseProfileData>());
  ASSERT_TRUE(n->get<PiraOneData>());
  n->get<PiraOneData>()->setNumberOfStatements(42);
  ASSERT_EQ(42, n->get<PiraOneData>()->getNumberOfStatements());
}

TEST_F(CgNodeBasics, CreateChildNodeDefaults) {
  auto n = std::make_shared<CgNode>("parent");
  auto c = std::make_shared<CgNode>("child");
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

TEST_F(CgNodeBasics, CreateChildParentRuntime) {
  auto n = std::make_shared<CgNode>("parent");
  auto c = std::make_shared<CgNode>("child");
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
  //ASSERT_EQ(1.5, CgHelper::calcInclusiveRuntime(n.get()));
  ASSERT_EQ(true, c->isLeafNode());
  ASSERT_EQ(true, n->isRootNode());
  ASSERT_EQ(false, n->isLeafNode());
  ASSERT_EQ(false, c->isRootNode());
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
}  // namespace

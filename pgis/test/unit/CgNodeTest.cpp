#include "CgNode.h"

#include "gtest/gtest.h"

namespace {
TEST(CgNodeBasics, CreateNodeDefaults) {
  auto n = std::make_shared<CgNode>("foo");
  ASSERT_STREQ("foo", n->getFunctionName().c_str());
  ASSERT_EQ(-1, n->getLineNumber());
  ASSERT_EQ(0.0, n->getRuntimeInSeconds());
  ASSERT_EQ(.0, n->getInclusiveRuntimeInSeconds());
  ASSERT_EQ(0, n->getNumberOfCalls());
  ASSERT_EQ(0, n->getNumberOfStatements());
  ASSERT_EQ(false, n->inPreviousProfile());
  ASSERT_EQ(false, n->hasUniqueCallPath());
  ASSERT_EQ(0, n->getParentNodes().size());
  ASSERT_EQ(0, n->getChildNodes().size());
  ASSERT_EQ(0, n->getSpantreeParents().size());
  ASSERT_EQ(true, n->isLeafNode());
  ASSERT_EQ(true, n->isRootNode());
}

TEST(CgNodeBasics, CreateSingleNodeWithRuntime) {
  auto n = std::make_shared<CgNode>("foo");
  n->setRuntimeInSeconds(1.23);
  ASSERT_EQ(1.23, n->getRuntimeInSeconds());
  // This is true for nodes w/o children
  ASSERT_EQ(1.23, n->getInclusiveRuntimeInSeconds());
}

TEST(CgNodeBasics, CreateSingleNodeWithStatements) {
  auto n = std::make_shared<CgNode>("foo");
  n->setNumberOfStatements(42);
  ASSERT_EQ(42, n->getNumberOfStatements());
}

TEST(CgNodeBasics, CreateChildNodeDefaults) {
  auto n = std::make_shared<CgNode>("parent");
  auto c = std::make_shared<CgNode>("child");
  ASSERT_EQ(0, n->getChildNodes().size());
  ASSERT_EQ(0, c->getParentNodes().size());
  n->addChildNode(c);
  ASSERT_EQ(1, n->getChildNodes().size());
  ASSERT_EQ(0, c->getParentNodes().size());
  ASSERT_TRUE(c->isSameFunction(*(n->getChildNodes().begin())));
}

TEST(CgNodeBasics, CreateChildParentNodeDefaults) {
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

TEST(CgNodeBasics, CreateChildParentRuntime) {
  auto n = std::make_shared<CgNode>("parent");
  auto c = std::make_shared<CgNode>("child");
  c->setComesFromCube();
  n->setComesFromCube();
  n->setRuntimeInSeconds(1.25);
  c->setRuntimeInSeconds(0.25);
  n->addChildNode(c);
  c->addParentNode(n);
  ASSERT_EQ(1.25, n->getRuntimeInSeconds());
  ASSERT_EQ(0.25, c->getRuntimeInSeconds());
  ASSERT_EQ(1.5, n->getInclusiveRuntimeInSeconds());
  ASSERT_EQ(true, c->isLeafNode());
  ASSERT_EQ(true, n->isRootNode());
  ASSERT_EQ(false, n->isLeafNode());
  ASSERT_EQ(false, c->isRootNode());
}

TEST(CgNodeBasics, CreateChildParentRemove) {
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

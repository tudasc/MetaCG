/**
 * File: MergePolicyTest.cpp
 * License: Part of the MetaCG project. Licensed under BSD 3 clause license. See LICENSE.txt file at
 * https://github.com/tudasc/metacg/LICENSE.txt
 */

#include "metacg/MergePolicy.h"
#include "metacg/Callgraph.h"

#include "gtest/gtest.h"

TEST(MergeByNameAndOrigin, NoNameMatch) {
  metacg::Callgraph target;
  target.insert("foo", "a.cpp", false, true);

  metacg::Callgraph source;
  auto& bar = source.insert("bar", "a.cpp", false, true);

  EXPECT_FALSE(metacg::MergeByNameAndOrigin().findMatchingNode(target, bar).has_value());
}

TEST(MergeByNameAndOrigin, NameMatchesOriginDiffers) {
  metacg::Callgraph target;
  target.insert("foo", "a.cpp", false, true);

  metacg::Callgraph source;
  auto& foo = source.insert("foo", "b.cpp", false, true);

  EXPECT_FALSE(metacg::MergeByNameAndOrigin().findMatchingNode(target, foo).has_value());
  // MergeByName would match these nodes
  EXPECT_TRUE(metacg::MergeByName().findMatchingNode(target, foo).has_value());
}

TEST(MergeByNameAndOrigin, OneOriginMissing) {
  metacg::Callgraph target;
  target.insert("foo", "a.cpp", false, true);

  metacg::Callgraph source;
  auto& foo = source.insert("foo", std::nullopt, false, false);

  EXPECT_FALSE(metacg::MergeByNameAndOrigin().findMatchingNode(target, foo).has_value());
}

TEST(MergeByNameAndOrigin, BothOriginsMissing) {
  metacg::Callgraph target;
  auto& targetFoo = target.insert("foo", std::nullopt, false, true);

  metacg::Callgraph source;
  auto& foo = source.insert("foo", std::nullopt, false, false);

  auto action = metacg::MergeByNameAndOrigin().findMatchingNode(target, foo);
  ASSERT_TRUE(action.has_value());
  EXPECT_EQ(action->targetNode, targetFoo.getId());
}

TEST(MergeByNameAndOrigin, SelectsNodeWithMatchingOrigin) {
  metacg::Callgraph target;
  target.insert("foo", "a.cpp", false, true);
  auto& targetFooB = target.insert("foo", "b.cpp", false, true);

  metacg::Callgraph source;
  auto& foo = source.insert("foo", "b.cpp", false, false);

  auto action = metacg::MergeByNameAndOrigin().findMatchingNode(target, foo);
  ASSERT_TRUE(action.has_value());
  EXPECT_EQ(action->targetNode, targetFooB.getId());
  EXPECT_FALSE(action->replace);
}

TEST(MergeByNameAndOrigin, ReplaceDeclarationWithDefinition) {
  metacg::Callgraph target;
  auto& targetFoo = target.insert("foo", "a.cpp", false, false);

  metacg::Callgraph source;
  auto& foo = source.insert("foo", "a.cpp", false, true);

  auto action = metacg::MergeByNameAndOrigin().findMatchingNode(target, foo);
  ASSERT_TRUE(action.has_value());
  EXPECT_EQ(action->targetNode, targetFoo.getId());
  EXPECT_TRUE(action->replace);
}

TEST(MergeByNameAndOrigin, MergeKeepsDifferentOriginsSeparate) {
  metacg::Callgraph target;
  auto& mainA = target.insert("main", "a.cpp", false, true);
  auto& fooA = target.insert("foo", "a.cpp", false, true);
  target.addEdge(mainA, fooA);

  metacg::Callgraph source;
  auto& barB = source.insert("bar", "b.cpp", false, true);
  auto& fooB = source.insert("foo", "b.cpp", false, true);
  source.addEdge(barB, fooB);

  target.merge(source, metacg::MergeByNameAndOrigin());

  EXPECT_EQ(target.getNodeCount(), 4);
  EXPECT_EQ(target.getAllNodesWithName("foo").size(), 2);
}

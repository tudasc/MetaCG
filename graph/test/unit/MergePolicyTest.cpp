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

TEST(MergeByNameAndOrigin, HeaderDeclarationMatchesSourceDefinition) {
  metacg::Callgraph target;
  auto& targetFoo = target.insert("foo", "include/foo.h", false, false);

  metacg::Callgraph source;
  auto& foo = source.insert("foo", "src/foo.cpp", false, true);

  auto action = metacg::MergeByNameAndOrigin().findMatchingNode(target, foo);
  ASSERT_TRUE(action.has_value());
  EXPECT_EQ(action->targetNode, targetFoo.getId());
  EXPECT_TRUE(action->replace);
}

TEST(MergeByNameAndOrigin, SourceDefinitionMatchesHeaderDeclaration) {
  metacg::Callgraph target;
  auto& targetFoo = target.insert("foo", "foo.c", false, true);

  metacg::Callgraph source;
  auto& foo = source.insert("foo", "foo.hpp", false, false);

  auto action = metacg::MergeByNameAndOrigin().findMatchingNode(target, foo);
  ASSERT_TRUE(action.has_value());
  EXPECT_EQ(action->targetNode, targetFoo.getId());
  EXPECT_FALSE(action->replace);
}

TEST(MergeByNameAndOrigin, HeaderSourceStemMismatch) {
  metacg::Callgraph target;
  target.insert("foo", "foo.h", false, false);

  metacg::Callgraph source;
  auto& foo = source.insert("foo", "bar.cpp", false, true);

  EXPECT_FALSE(metacg::MergeByNameAndOrigin().findMatchingNode(target, foo).has_value());
}

TEST(MergeByNameAndOrigin, HeaderAndSourceBothWithBody) {
  metacg::Callgraph target;
  target.insert("foo", "foo.h", false, true);

  metacg::Callgraph source;
  auto& foo = source.insert("foo", "foo.cpp", false, true);

  EXPECT_FALSE(metacg::MergeByNameAndOrigin().findMatchingNode(target, foo).has_value());
}

TEST(MergeByNameAndOrigin, HeaderWithBodySourceWithoutBody) {
  metacg::Callgraph target;
  target.insert("foo", "foo.h", false, true);

  metacg::Callgraph source;
  auto& foo = source.insert("foo", "foo.cpp", false, false);

  EXPECT_FALSE(metacg::MergeByNameAndOrigin().findMatchingNode(target, foo).has_value());
}

TEST(MergeByNameAndOrigin, TwoSourceFilesNotMatched) {
  metacg::Callgraph target;
  target.insert("foo", "a/foo.cpp", false, false);

  metacg::Callgraph source;
  auto& foo = source.insert("foo", "b/foo.cpp", false, true);

  EXPECT_FALSE(metacg::MergeByNameAndOrigin().findMatchingNode(target, foo).has_value());
}

TEST(MergeByNameAndOrigin, ExactOriginPreferredOverHeaderSource) {
  metacg::Callgraph target;
  target.insert("foo", "foo.cpp", false, true);
  auto& targetHeaderFoo = target.insert("foo", "foo.h", false, false);

  metacg::Callgraph source;
  auto& foo = source.insert("foo", "foo.h", false, false);

  auto action = metacg::MergeByNameAndOrigin().findMatchingNode(target, foo);
  ASSERT_TRUE(action.has_value());
  EXPECT_EQ(action->targetNode, targetHeaderFoo.getId());
}

TEST(MergeByNameAndOrigin, MergeReplacesHeaderDeclarationWithSourceDefinition) {
  metacg::Callgraph target;
  auto& mainA = target.insert("main", "main.cpp", false, true);
  auto& fooDecl = target.insert("foo", "foo.h", false, false);
  target.addEdge(mainA, fooDecl);

  metacg::Callgraph source;
  source.insert("foo", "foo.cpp", false, true);

  target.merge(source, metacg::MergeByNameAndOrigin());

  EXPECT_EQ(target.getNodeCount(), 2);
  auto& fooIds = target.getAllNodesWithName("foo");
  ASSERT_EQ(fooIds.size(), 1);
  auto* fooNode = target.getNode(fooIds.front());
  EXPECT_TRUE(fooNode->getHasBody());
  EXPECT_EQ(fooNode->getOrigin(), "foo.cpp");
}

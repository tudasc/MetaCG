/**
* File: DotIOTest.cpp
* License: Part of the MetaCG project. Licensed under BSD 3 clause license. See LICENSE.txt file at
* https://github.com/tudasc/metacg/LICENSE.txt
 */

#include "LoggerUtil.h"
#include "gtest/gtest.h"

#include "DotIO.h"
#include "MCGManager.h"

class DotIOTest : public ::testing::Test {
 protected:
  void SetUp() override {
    metacg::loggerutil::getLogger();
    auto &mcgm = metacg::graph::MCGManager::get();
    mcgm.resetManager();
    mcgm.addToManagedGraphs("emptyGraph", std::make_unique<metacg::Callgraph>());
  }
};

TEST_F(DotIOTest, EmptyCGExport) {
  const auto cg = metacg::graph::MCGManager::get().getCallgraph();
  metacg::io::dot::DotGenerator generator(cg);
  generator.generate();
  const auto dotStr = generator.getDotString();
  EXPECT_EQ(dotStr, "digraph callgraph {\n}\n");
}

TEST_F(DotIOTest, OneNodeCGExport) {
  const auto &cg = metacg::graph::MCGManager::get().getCallgraph();
  cg->getOrInsertNode("main");
  metacg::io::dot::DotGenerator generator(cg);
  generator.generate();
  const auto dotStr = generator.getDotString();
  EXPECT_EQ(dotStr, "digraph callgraph {\n  \"main\"\n}\n");
}

/**
 * One of the main issues that we have currently is that for some of our tests we assume a certain sorting in the
 * container. This is sub-optimal and we should find a better solution.
 */
TEST_F(DotIOTest, TwoNodeNoEdgeCGExport) {
  const auto cg = metacg::graph::MCGManager::get().getCallgraph();
  cg->getOrInsertNode("foo");
  cg->getOrInsertNode("bar");
  metacg::io::dot::DotGenerator generator(cg);
  generator.generate();
  const auto dotStr = generator.getDotString();
  EXPECT_EQ(dotStr, "digraph callgraph {\n  \"bar\"\n  \"foo\"\n}\n");
}

TEST_F(DotIOTest, TwoNodeOneEdgeCGExport) {
  const auto cg = metacg::graph::MCGManager::get().getCallgraph();
  auto parent = cg->getOrInsertNode("foo");
  auto child = cg->getOrInsertNode("bar");
  cg->addEdge(parent, child);
  metacg::io::dot::DotGenerator generator(cg);
  generator.generate();
  const auto dotStr = generator.getDotString();
  EXPECT_EQ(dotStr, "digraph callgraph {\n  \"bar\"\n  \"foo\"\n\n  foo -> bar\n}\n");
}

TEST_F(DotIOTest, ThreeNodeOneEdgeCGExport) {
  const auto cg = metacg::graph::MCGManager::get().getCallgraph();
  auto unreachableChild = cg->getOrInsertNode("unreachable");
  auto parent = cg->getOrInsertNode("foo");
  auto child = cg->getOrInsertNode("bar");
  cg->addEdge(parent, child);
  metacg::io::dot::DotGenerator generator(cg);
  generator.generate();
  const auto dotStr = generator.getDotString();
  EXPECT_EQ(dotStr, "digraph callgraph {\n  \"bar\"\n  \"foo\"\n  \"unreachable\"\n\n  foo -> bar\n}\n");
}

TEST_F(DotIOTest, ThreeNodeTwoEdgeCGExport) {
  const auto cg = metacg::graph::MCGManager::get().getCallgraph();
  auto c3 = cg->getOrInsertNode("c3");
  auto c2 = cg->getOrInsertNode("c2");
  auto c1 = cg->getOrInsertNode("c1");
  cg->addEdge(c1, c2);
  cg->addEdge(c2, c3);
  metacg::io::dot::DotGenerator generator(cg);
  generator.generate();
  const auto dotStr = generator.getDotString();
  EXPECT_EQ(dotStr, "digraph callgraph {\n  \"c1\"\n  \"c2\"\n  \"c3\"\n\n  c2 -> c3\n  c1 -> c2\n}\n");
}

TEST_F(DotIOTest, NoMultipleEdgesExport) {
  const auto cg = metacg::graph::MCGManager::get().getCallgraph();
  auto c4 = cg->getOrInsertNode("c4");
  auto c3 = cg->getOrInsertNode("c3");
  auto c2 = cg->getOrInsertNode("c2");
  auto c1 = cg->getOrInsertNode("c1");
  cg->addEdge(c1, c2);
  cg->addEdge(c3, c2);
  cg->addEdge(c2, c4);
  metacg::io::dot::DotGenerator generator(cg);
  generator.generate();
  const auto dotStr = generator.getDotString();
  EXPECT_EQ(dotStr,
            "digraph callgraph {\n  \"c1\"\n  \"c2\"\n  \"c3\"\n  \"c4\"\n\n  c2 -> c4\n  c3 -> c2\n  c1 -> c2\n}\n");
}

TEST_F(DotIOTest, TwoNodesWithCycleCGExport) {
  const auto cg = metacg::graph::MCGManager::get().getCallgraph();
  auto c2 = cg->getOrInsertNode("c2");
  auto c1 = cg->getOrInsertNode("c1");
  cg->addEdge(c1, c2);
  cg->addEdge(c2, c1);
  metacg::io::dot::DotGenerator generator(cg);
  generator.generate();
  const auto dotStr = generator.getDotString();
  EXPECT_EQ(dotStr, "digraph callgraph {\n  \"c1\"\n  \"c2\"\n\n  c1 -> c2\n  c2 -> c1\n}\n");
}

TEST_F(DotIOTest, ThreeNodesWithCycleCGExport) {
  const auto cg = metacg::graph::MCGManager::get().getCallgraph();
  auto c3 = cg->getOrInsertNode("c3");
  auto c2 = cg->getOrInsertNode("c2");
  auto c1 = cg->getOrInsertNode("c1");
  cg->addEdge(c1, c2);
  cg->addEdge(c2, c3);
  cg->addEdge(c3, c1);
  metacg::io::dot::DotGenerator generator(cg);
  generator.generate();
  const auto dotStr = generator.getDotString();
  EXPECT_EQ(dotStr, "digraph callgraph {\n  \"c1\"\n  \"c2\"\n  \"c3\"\n\n  c2 -> c3\n  c1 -> c2\n  c3 -> c1\n}\n");
}

TEST_F(DotIOTest, ReadEmptyDot) {
  const std::string dotStr{"digraph callgraph {\n}\n"};
  auto readerSrc = metacg::io::dot::DotStringSource(dotStr);
  metacg::io::dot::DotReader reader(metacg::graph::MCGManager::get(), readerSrc);
  auto created = reader.readAndManage("empty");
  EXPECT_EQ(created, true);
}

TEST_F(DotIOTest, ReadOneNodeDot) {
  const std::string dotStr{"digraph callgraph {\n\"node_one\"}\n"};
  auto readerSrc = metacg::io::dot::DotStringSource(dotStr);
  auto &manager = metacg::graph::MCGManager::get();
  metacg::io::dot::DotReader reader(manager, readerSrc);
  auto created = reader.readAndManage("empty");
  EXPECT_EQ(created, true);
  auto graph = manager.getCallgraph();
  EXPECT_TRUE(graph->hasNode("node_one"));
}

TEST_F(DotIOTest, ReadOneNodeDotWithLeadingSpaces) {
  const std::string dotStr{"digraph callgraph {\n   \"node_one\"}\n"};
  auto readerSrc = metacg::io::dot::DotStringSource(dotStr);
  auto &manager = metacg::graph::MCGManager::get();
  metacg::io::dot::DotReader reader(manager, readerSrc);
  auto created = reader.readAndManage("empty");
  EXPECT_EQ(created, true);
  auto graph = manager.getCallgraph();
  EXPECT_TRUE(graph->hasNode("node_one"));
}

TEST_F(DotIOTest, ReadTwoNodeDotNoEdge) {
  const std::string dotStr{"digraph callgraph {\n\"node_one\"\n    \"node_two\"\n}\n"};
  auto readerSrc = metacg::io::dot::DotStringSource(dotStr);
  auto &manager = metacg::graph::MCGManager::get();
  metacg::io::dot::DotReader reader(manager, readerSrc);
  auto created = reader.readAndManage("empty");
  EXPECT_EQ(created, true);
  auto graph = manager.getCallgraph();
  EXPECT_TRUE(graph->hasNode("node_one"));
  EXPECT_TRUE(graph->hasNode("node_two"));

  EXPECT_EQ(graph->getCallees("node_one").size(), 0);
  EXPECT_EQ(graph->getCallees("node_two").size(), 0);
  EXPECT_EQ(graph->getCallers("node_one").size(), 0);
  EXPECT_EQ(graph->getCallers("node_two").size(), 0);
}

TEST_F(DotIOTest, ReadTwoNodeDotWithEdge) {
  const std::string dotStr{"digraph callgraph {\n\"node_one\"\n    \"node_two\"\n\n \"node_one\" -> \"node_two\"\n}\n"};
  auto readerSrc = metacg::io::dot::DotStringSource(dotStr);
  auto &manager = metacg::graph::MCGManager::get();
  metacg::io::dot::DotReader reader(manager, readerSrc);
  auto created = reader.readAndManage("empty");
  EXPECT_EQ(created, true);
  auto graph = manager.getCallgraph();

  EXPECT_TRUE(graph->hasNode("node_one"));
  auto nodeOne = graph->getNode("node_one");
  EXPECT_EQ(graph->getCallees(nodeOne->getId()).size(), 1);

  EXPECT_TRUE(graph->hasNode("node_two"));
  auto nodeTwo = graph->getNode("node_two");
  EXPECT_EQ(graph->getCallers(nodeTwo->getId()).size(), 1);
}

TEST(DotTokenizerTest, InitialTest) {
  const std::string initialStr{"digraph callgraph {\nn\n1\n}\n"};
  metacg::io::dot::Tokenizer tok;
  auto strV = tok.splitToTokenStrings(initialStr);
  std::vector<std::string> cmpV{"digraph", "callgraph", "{", "n", "1", "}"};
  EXPECT_EQ(cmpV, strV);
}

TEST(DotTokenizerTest, InitialTestOne) {
  const std::string initialStr{"digraph callgraph {\nnode\n1\n}\n"};
  metacg::io::dot::Tokenizer tok;
  auto strV = tok.splitToTokenStrings(initialStr);
  std::vector<std::string> cmpV{"digraph", "callgraph", "{", "node", "1", "}"};
  EXPECT_EQ(cmpV, strV);
}

TEST(DotTokenizerTest, InitialTestTwo) {
  const std::string initialStr{"digraph callgraph {\nn\n1\n1n\nn1\n}\n"};
  metacg::io::dot::Tokenizer tok;
  auto strV = tok.splitToTokenStrings(initialStr);
  std::vector<std::string> cmpV{"digraph", "callgraph", "{", "n", "1", "1", "n", "n1", "}"};
  EXPECT_EQ(cmpV, strV);
}

TEST(DotTokenizerTest, InitialTestThree) {
  const std::string initialStr{"digraph callgraph {\nn [attribute=\"asdf\"]\n}\n"};
  metacg::io::dot::Tokenizer tok;
  auto strV = tok.splitToTokenStrings(initialStr);
  std::vector<std::string> cmpV{"digraph", "callgraph", "{", "n", "[", "attribute", "=", "\"", "asdf", "\"", "]", "}"};
  EXPECT_EQ(cmpV, strV);
}

TEST(DotTokenizerTest, InitialTestFour) {
  const std::string initialStr{"digraph callgraph {n [attribute=\"asdf\"]\n}\n"};
  metacg::io::dot::Tokenizer tok;
  auto strV = tok.splitToTokenStrings(initialStr);
  std::vector<std::string> cmpV{"digraph", "callgraph", "{", "n", "[", "attribute", "=", "\"", "asdf", "\"", "]", "}"};
  EXPECT_EQ(cmpV, strV);
}

TEST(DotTokenizerTest, InitialTestFive) {
  const std::string initialStr{"digraph callgraph {\n1n\n112\n}\n"};
  metacg::io::dot::Tokenizer tok;
  auto strV = tok.splitToTokenStrings(initialStr);
  std::vector<std::string> cmpV{"digraph", "callgraph", "{", "1", "n", "112", "}"};
  EXPECT_EQ(cmpV, strV);
}

TEST(DotTokenizerTest, InitialTestSix) {
  const std::string initialStr{"digraph callgraph {\n1\n1n\n112\n}\n"};
  metacg::io::dot::Tokenizer tok;
  auto strV = tok.splitToTokenStrings(initialStr);
  std::vector<std::string> cmpV{"digraph", "callgraph", "{", "1", "1", "n", "112", "}"};
  EXPECT_EQ(cmpV, strV);
}

TEST(DotTokenizerTest, InitialTestSeven) {
  const std::string initialStr{"digraph callgraph {a -> b}\n"};
  metacg::io::dot::Tokenizer tok;
  auto strV = tok.splitToTokenStrings(initialStr);
  std::vector<std::string> cmpV{"digraph", "callgraph", "{", "a", "->", "b", "}"};
  EXPECT_EQ(cmpV, strV);
}

TEST(DotTokenizerTest, InitialTestEight) {
  const std::string initialStr{"digraph callgraph {a -> b -> c}\n"};
  metacg::io::dot::Tokenizer tok;
  auto strV = tok.splitToTokenStrings(initialStr);
  std::vector<std::string> cmpV{"digraph", "callgraph", "{", "a", "->", "b", "->", "c", "}"};
  EXPECT_EQ(cmpV, strV);
}

TEST(DotTokenizerTest, TokenStreamTest) {
  const std::string initialStr{"digraph callgraph {a -> b -> c}\n"};
  metacg::io::dot::Tokenizer tok;
  auto tokStream = tok.tokenize(initialStr);
  using TT = metacg::io::dot::ParsedToken::TokenType;
  std::vector<metacg::io::dot::ParsedToken> expectedToks{
      {TT::ENTITY, "digraph"}, {TT::ENTITY, "callgraph"}, {TT::IGNORE, "{"}, {TT::ENTITY, "a"}, {TT::CONNECTOR, "->"},
      {TT::ENTITY, "b"},       {TT::CONNECTOR, "->"},     {TT::ENTITY, "c"}, {TT::IGNORE, "}"}};
  EXPECT_EQ(tokStream, expectedToks);
}

TEST(DotParserTest, OneNodeTest) {
  const std::string initStr{"digraph callgraph {\na\n}\n"};
  auto cg = std::make_unique<metacg::Callgraph>();
  metacg::io::dot::DotParser parser(cg.get());
  parser.parse(initStr);
  EXPECT_EQ(cg->size(), 1);
}

TEST(DotParserTest, TwoNodeTest) {
  const std::string initStr{"digraph callgraph {\na\n1\n}\n"};
  auto cg = std::make_unique<metacg::Callgraph>();
  metacg::io::dot::DotParser parser(cg.get());
  parser.parse(initStr);
  EXPECT_EQ(cg->size(), 2);
}

TEST(DotParserTest, ThreeNodeTest) {
  const std::string initStr{"digraph callgraph {\na\n1m\n}\n"};
  auto cg = std::make_unique<metacg::Callgraph>();
  metacg::io::dot::DotParser parser(cg.get());
  parser.parse(initStr);
  EXPECT_EQ(cg->size(), 3);
}

TEST(DotParserTest, OneEdgeWithWhitespaceTest) {
  const std::string initStr{"digraph callgraph {\na       ->\tb\n}\n"};
  auto cg = std::make_unique<metacg::Callgraph>();
  metacg::io::dot::DotParser parser(cg.get());
  parser.parse(initStr);
  EXPECT_EQ(cg->size(), 2);
  auto aNode = cg->getNode("a");
  auto bNode = cg->getNode("b");
  auto aChildNodes = cg->getCallees(aNode->getId());
  EXPECT_TRUE(aChildNodes.find(bNode) != aChildNodes.end());
}

TEST(DotParserTest, OneEdgeAllNewLineTest) {
  const std::string initStr{"digraph callgraph {\na\n->\nb\n}\n"};
  auto cg = std::make_unique<metacg::Callgraph>();
  metacg::io::dot::DotParser parser(cg.get());
  parser.parse(initStr);
  EXPECT_EQ(cg->size(), 2);
  auto aNode = cg->getNode("a");
  auto bNode = cg->getNode("b");
  auto aChildNodes = cg->getCallees(aNode->getId());
  EXPECT_TRUE(aChildNodes.find(bNode) != aChildNodes.end());
}

TEST(DotParserTest, OneEdgeTest) {
  const std::string initStr{"digraph callgraph {\na -> b\n}\n"};
  auto cg = std::make_unique<metacg::Callgraph>();
  metacg::io::dot::DotParser parser(cg.get());
  parser.parse(initStr);
  EXPECT_EQ(cg->size(), 2);
  auto aNode = cg->getNode("a");
  auto bNode = cg->getNode("b");
  auto aChildNodes = cg->getCallees(aNode->getId());
  EXPECT_TRUE(aChildNodes.find(bNode) != aChildNodes.end());
}

TEST(DotParserTest, MultiNodeOneEdgeTest) {
  const std::string initStr{"digraph callgraph {\nn\nzz\na -> b\n}\n"};
  auto cg = std::make_unique<metacg::Callgraph>();
  metacg::io::dot::DotParser parser(cg.get());
  parser.parse(initStr);
  EXPECT_EQ(cg->size(), 4);
  EXPECT_TRUE(cg->hasNode("n"));
  EXPECT_TRUE(cg->hasNode("zz"));
  EXPECT_TRUE(cg->hasNode("a"));
  EXPECT_TRUE(cg->hasNode("b"));
  auto aNode = cg->getNode("a");
  auto bNode = cg->getNode("b");
  auto aChildNodes = cg->getCallees(aNode->getId());
  EXPECT_TRUE(aChildNodes.find(bNode) != aChildNodes.end());
}
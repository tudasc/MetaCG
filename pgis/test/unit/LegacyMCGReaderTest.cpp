/**
 * File: LegacyMCGReaderTest.cpp
 * License: Part of the metacg project. Licensed under BSD 3 clause license. See LICENSE.txt file at
 * https://github.com/tudasc/metacg/LICENSE.txt
 */

#include "LegacyMCGReader.h"
#include "LoggerUtil.h"
#include "MCGManager.h"
#include "loadImbalance/LIMetaData.h"
#include "gtest/gtest.h"

#include "nlohmann/json.hpp"

using namespace metacg;
using json = nlohmann::json;

class VersionOneMCGReaderTest : public ::testing::Test {
 protected:
  void SetUp() override {
    metacg::loggerutil::getLogger();
    auto& mcgm = metacg::graph::MCGManager::get();
    mcgm.resetManager();
  }
};

TEST(VersionOneMCGReaderTest, EmptyJSON) {
  const json j;
  metacg::loggerutil::getLogger();
  auto& mcgm = metacg::graph::MCGManager::get();
  metacg::io::JsonSource js(j);
  metacg::pgis::io::VersionOneMCGReader mcgReader(js);
  mcgm.addToManagedGraphs("emptyGraph", std::make_unique<Callgraph>());
  mcgReader.read();

  const Callgraph& graph = *mcgm.getCallgraph();
  ASSERT_EQ(graph.size(), 0);
}

TEST(VersionOneMCGReaderTest, SimpleJSON) {
  json j;
  j["main"] = {
      {"numStatements", 42},      {"doesOverride", false},         {"hasBody", true},
      {"isVirtual", false},       {"overriddenBy", json::array()}, {"overriddenFunctions", json::array()},
      {"parents", json::array()}, {"callees", json::array()},

  };

  auto& mcgm = metacg::graph::MCGManager::get();
  metacg::io::JsonSource js(j);
  metacg::pgis::io::VersionOneMCGReader mcgReader(js);
  mcgm.addToManagedGraphs("emptyGraph", std::make_unique<Callgraph>());
  mcgReader.read();

  const Callgraph& graph = *mcgm.getCallgraph();
  EXPECT_EQ(graph.size(), 1);

  metacg::CgNode* mainNode = &graph.getSingleNode("main");
  mainNode->getOrCreate<pira::PiraOneData>();
  mainNode->getOrCreate<pira::PiraTwoData>();
  mainNode->getOrCreate<LoadImbalance::LIMetaData>();
  ASSERT_NE(mainNode, nullptr);

  EXPECT_EQ(mainNode->getFunctionName(), "main");
  EXPECT_EQ(mainNode->get<pira::PiraOneData>()->getNumberOfStatements(), 42);
  EXPECT_EQ(mainNode->get<pira::PiraOneData>()->getHasBody(), true);
  EXPECT_EQ(mainNode->get<LoadImbalance::LIMetaData>()->isVirtual(), false);
}

TEST(VersionOneMCGReaderTest, MultiNodeJSON) {
  json j;
  j["main"] = {
      {"numStatements", 42},
      {"doesOverride", false},
      {"hasBody", true},
      {"isVirtual", false},
      {"overriddenBy", json::array()},
      {"overriddenFunctions", json::array()},
      {"parents", json::array()},
      {"callees", {"foo"}},
  };

  j["foo"] = {
      {"numStatements", 1},  {"doesOverride", false},         {"hasBody", true},
      {"isVirtual", false},  {"overriddenBy", json::array()}, {"overriddenFunctions", json::array()},
      {"parents", {"main"}}, {"callees", json::array()},
  };

  auto& mcgm = metacg::graph::MCGManager::get();
  metacg::io::JsonSource js(j);
  metacg::pgis::io::VersionOneMCGReader mcgReader(js);
  mcgm.addToManagedGraphs("emptyGraph", std::make_unique<Callgraph>());
  mcgReader.read();

  const Callgraph& graph = *mcgm.getCallgraph();
  EXPECT_EQ(graph.size(), 2);

  metacg::CgNode* mainNode = &graph.getSingleNode("main");
  ASSERT_NE(mainNode, nullptr);
  mainNode->getOrCreate<pira::PiraOneData>();
  mainNode->getOrCreate<pira::PiraTwoData>();
  mainNode->getOrCreate<LoadImbalance::LIMetaData>();
  EXPECT_EQ(mainNode->getFunctionName(), "main");
  EXPECT_EQ(mainNode->get<pira::PiraOneData>()->getNumberOfStatements(), 42);
  EXPECT_EQ(mainNode->get<pira::PiraOneData>()->getHasBody(), true);
  EXPECT_EQ(mainNode->get<LoadImbalance::LIMetaData>()->isVirtual(), false);
  EXPECT_EQ(graph.getCallers(*mainNode).size(), 0);
  EXPECT_EQ(graph.getCallees(*mainNode).size(), 1);
  for (const auto cn : graph.getCallees(*mainNode)) {
    EXPECT_EQ(cn->getFunctionName(), "foo");
  }

  metacg::CgNode* fooNode = &graph.getSingleNode("foo");
  ASSERT_NE(fooNode, nullptr);

  fooNode->getOrCreate<pira::PiraOneData>();
  fooNode->getOrCreate<pira::PiraTwoData>();
  fooNode->getOrCreate<LoadImbalance::LIMetaData>();
  EXPECT_EQ(fooNode->getFunctionName(), "foo");
  EXPECT_EQ(fooNode->get<pira::PiraOneData>()->getNumberOfStatements(), 1);
  EXPECT_EQ(fooNode->get<pira::PiraOneData>()->getHasBody(), true);
  EXPECT_EQ(fooNode->get<LoadImbalance::LIMetaData>()->isVirtual(), false);
  EXPECT_EQ(graph.getCallees(*fooNode).size(), 0);
  EXPECT_EQ(graph.getCallers(*fooNode).size(), 1);
  for (const auto pn : graph.getCallers(*fooNode)) {
    EXPECT_EQ(pn->getFunctionName(), "main");
  }
}

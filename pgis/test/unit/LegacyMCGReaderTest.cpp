/**
* File: LegacyMCGReaderTest.cpp
* License: Part of the metacg project. Licensed under BSD 3 clause license. See LICENSE.txt file at
* https://github.com/tudasc/metacg/LICENSE.txt
*/

#include "gtest/gtest.h"
#include "LoggerUtil.h"
#include "MCGManager.h"
#include "LegacyMCGReader.h"
#include "MetaDataHandler.h"
#include "loadImbalance/LIMetaData.h"

#include "nlohmann/json.hpp"

using namespace metacg;
using json = nlohmann::json;

/**
* MetaDataHandler used for testing
*/
struct TestHandler : public metacg::io::retriever::MetaDataHandler {
 int i{0};
 const std::string toolName() const override { return "TestMetaHandler"; }
 void read([[maybe_unused]] const json &j, const std::string &functionName) override { i++; }
 bool handles(const metacg::CgNode* n) const override { return false; }
 json value(const metacg::CgNode* n) const override {
   json j;
   j = i;
   return j;
 }
};


TEST(VersionOneMCGReaderTest, EmptyJSON) {
  json j;
  metacg::loggerutil::getLogger();

  auto &mcgm = metacg::graph::MCGManager::get();
  mcgm.resetManager();
  mcgm.addToManagedGraphs("emptyGraph",std::make_unique<metacg::Callgraph>());
  metacg::io::JsonSource js(j);
  metacg::io::VersionOneMetaCGReader mcgReader(js);
  mcgReader.read(mcgm);

  const Callgraph &graph = *mcgm.getCallgraph();
  ASSERT_EQ(graph.size(), 0);
}

TEST(VersionOneMCGReaderTest, SimpleJSON) {
  json j;
  j["main"] = {
      {"numStatements", 42},      {"doesOverride", false},         {"hasBody", true},
      {"isVirtual", false},       {"overriddenBy", json::array()}, {"overriddenFunctions", json::array()},
      {"parents", json::array()}, {"callees", json::array()},

  };

  auto &mcgm = metacg::graph::MCGManager::get();
  mcgm.resetManager();
  mcgm.addToManagedGraphs("emptyGraph",std::make_unique<metacg::Callgraph>());
  metacg::io::JsonSource js(j);
  metacg::io::VersionOneMetaCGReader mcgReader(js);
  mcgReader.read(mcgm);

  Callgraph &graph = *mcgm.getCallgraph();
  EXPECT_EQ(graph.size(), 1);

  metacg::CgNode* mainNode = graph.getNode("main");
  mainNode->getOrCreateMD<pira::PiraOneData>();
  mainNode->getOrCreateMD<pira::PiraTwoData>();
  mainNode->getOrCreateMD<LoadImbalance::LIMetaData>();
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

  auto &mcgm = metacg::graph::MCGManager::get();
  mcgm.resetManager();
  mcgm.addToManagedGraphs("emptyGraph",std::make_unique<metacg::Callgraph>());
  metacg::io::JsonSource js(j);
  metacg::io::VersionOneMetaCGReader mcgReader(js);
  mcgReader.read(mcgm);

  Callgraph &graph = *mcgm.getCallgraph();
  EXPECT_EQ(graph.size(), 2);

  metacg::CgNode* mainNode = graph.getNode("main");
  ASSERT_NE(mainNode, nullptr);
  mainNode->getOrCreateMD<pira::PiraOneData>();
  mainNode->getOrCreateMD<pira::PiraTwoData>();
  mainNode->getOrCreateMD<LoadImbalance::LIMetaData>();
  EXPECT_EQ(mainNode->getFunctionName(), "main");
  EXPECT_EQ(mainNode->get<pira::PiraOneData>()->getNumberOfStatements(), 42);
  EXPECT_EQ(mainNode->get<pira::PiraOneData>()->getHasBody(), true);
  EXPECT_EQ(mainNode->get<LoadImbalance::LIMetaData>()->isVirtual(), false);
  EXPECT_EQ(graph.getCallers(mainNode).size(), 0);
  EXPECT_EQ(graph.getCallees(mainNode).size(), 1);
  for (const auto cn : graph.getCallees(mainNode)) {
    EXPECT_EQ(cn->getFunctionName(), "foo");
  }

  metacg::CgNode* fooNode = graph.getNode("foo");
  ASSERT_NE(fooNode, nullptr);

  fooNode->getOrCreateMD<pira::PiraOneData>();
  fooNode->getOrCreateMD<pira::PiraTwoData>();
  fooNode->getOrCreateMD<LoadImbalance::LIMetaData>();
  EXPECT_EQ(fooNode->getFunctionName(), "foo");
  EXPECT_EQ(fooNode->get<pira::PiraOneData>()->getNumberOfStatements(), 1);
  EXPECT_EQ(fooNode->get<pira::PiraOneData>()->getHasBody(), true);
  EXPECT_EQ(fooNode->get<LoadImbalance::LIMetaData>()->isVirtual(), false);
  EXPECT_EQ(graph.getCallees(fooNode).size(), 0);
  EXPECT_EQ(graph.getCallers(fooNode).size(), 1);
  for (const auto pn : graph.getCallers(fooNode)) {
    EXPECT_EQ(pn->getFunctionName(), "main");
  }
}
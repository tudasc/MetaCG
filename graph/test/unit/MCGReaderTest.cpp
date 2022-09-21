/**
 * File: MCGReaderTest.cpp
 * License: Part of the metacg project. Licensed under BSD 3 clause license. See LICENSE.txt file at
 * https://github.com/tudasc/metacg/LICENSE.txt
 */

#include "gtest/gtest.h"

#include "LoggerUtil.h"
#include "MCGManager.h"
#include "MCGReader.h"
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
  bool handles(const CgNodePtr n) const override { return false; }
  json value(const CgNodePtr n) const {
    json j;
    j = i;
    return j;
  }
};


TEST(VersionTwoMetaCGReaderTest, EmptyJSON) {
  json j;
  metacg::loggerutil::getLogger();

  // No MetaData Reader added to CGManager
  auto &mcgm = metacg::graph::MCGManager::get();
  mcgm.resetManager();
  mcgm.addToManagedGraphs("emptyGraph",std::make_unique<metacg::Callgraph>());
  metacg::io::JsonSource js(j);
  metacg::io::VersionTwoMetaCGReader mcgReader(js);
  ASSERT_THROW(mcgReader.read(mcgm), std::runtime_error);
}

TEST(VersionTwoMetaCGReaderTest, EmptyCG) {
  metacg::loggerutil::getLogger();

  json j;
  j["_MetaCG"] = {{"version", "1.0"}, {"generator", {{"name", "testGen"}, {"version", "1.0"}}}};
  j["_CG"] = {};

  // No MetaData Reader added to CGManager
  auto &mcgm = metacg::graph::MCGManager::get();
  mcgm.resetManager();
  mcgm.addToManagedGraphs("emptyGraph",std::make_unique<metacg::Callgraph>());
  metacg::io::JsonSource js(j);
  metacg::io::VersionTwoMetaCGReader mcgReader(js);
  // "The call graph in the metacg file was null"
  ASSERT_THROW(mcgReader.read(mcgm), std::runtime_error);
}

TEST(VersionTwoMetaCGReaderTest, SingleMetaDataHandlerEmptyJSON) {
  json j;
  metacg::loggerutil::getLogger();

  auto &mcgm = metacg::graph::MCGManager::get();
  mcgm.resetManager();
  mcgm.addToManagedGraphs("emptyGraph",std::make_unique<metacg::Callgraph>());
  mcgm.addMetaHandler<TestHandler>();

  metacg::io::JsonSource js(j);
  metacg::io::VersionTwoMetaCGReader mcgReader(js);
  ASSERT_THROW(mcgReader.read(mcgm), std::runtime_error);
}

TEST(VersionTwoMetaCGReaderTest, OneNodeNoMetaDataHandler) {
  metacg::loggerutil::getLogger();

  json j;
  j["_MetaCG"] = {{"version", "1.0"}, {"generator", {{"name", "testGen"}, {"version", "1.0"}}}};
  j["_CG"] = {{"main",
               {{"doesOverride", false},
                {"hasBody", true},
                {"isVirtual", false},
                {"overriddenBy", json::array()},
                {"overrides", json::array()},
                {"callers", json::array()},
                {"callees", json::array()}}}};

  // No MetaData Reader added to CGManager
  auto &mcgm = metacg::graph::MCGManager::get();
  mcgm.resetManager();
  mcgm.addToManagedGraphs("emptyGraph",std::make_unique<metacg::Callgraph>());
  metacg::io::JsonSource js(j);
  metacg::io::VersionTwoMetaCGReader mcgReader(js);
  mcgReader.read(mcgm);

  auto graph = *mcgm.getCallgraph();
  EXPECT_EQ(graph.size(), 1);

  const auto mainNode = graph.getMain();
  EXPECT_EQ(mainNode->getFunctionName(), "main");
  EXPECT_EQ(mainNode->getChildNodes().size(), 0);
  EXPECT_EQ(mainNode->getParentNodes().size(), 0);
}

TEST(VersionTwoMetaCGReaderTest, TwoNodesNoMetaDataHandler) {
  metacg::loggerutil::getLogger();

  json j;
  j["_MetaCG"] = {{"version", "1.0"}, {"generator", {{"name", "testGen"}, {"version", "1.0"}}}};
  j["_CG"] = {{"main",
               {{"doesOverride", false},
                {"hasBody", true},
                {"isVirtual", false},
                {"overriddenBy", json::array()},
                {"overrides", json::array()},
                {"callers", json::array()},
                {"callees", {"foo"}},
                {"meta", {}}}},
              {"foo",
               {{"doesOverride", false},
                {"hasBody", true},
                {"isVirtual", false},
                {"overriddenBy", json::array()},
                {"overrides", json::array()},
                {"callers", {"main"}},
                {"callees", json::array()},
                {"meta", {}}}}};

  // No MetaData Reader added to CGManager
  auto &mcgm = metacg::graph::MCGManager::get();
  mcgm.resetManager();
  mcgm.addToManagedGraphs("emptyGraph",std::make_unique<metacg::Callgraph>());
  metacg::io::JsonSource js(j);
  metacg::io::VersionTwoMetaCGReader mcgReader(js);
  mcgReader.read(mcgm);

  auto graph = *mcgm.getCallgraph();
  EXPECT_EQ(graph.size(), 2);

  const auto mainNode = graph.getMain();
  EXPECT_EQ(mainNode->getFunctionName(), "main");
  EXPECT_EQ(mainNode->getChildNodes().size(), 1);
  EXPECT_EQ(mainNode->getParentNodes().size(), 0);
}

TEST(VersionTwoMetaCGReaderTest, TwoNodesOneMetaDataHandler) {
  metacg::loggerutil::getLogger();

  json j;
  j["_MetaCG"] = {{"version", "1.0"}, {"generator", {{"name", "testGen"}, {"version", "1.0"}}}};
  j["_CG"] = {{"main",
               {{"doesOverride", false},
                {"hasBody", true},
                {"isVirtual", false},
                {"overriddenBy", json::array()},
                {"overrides", json::array()},
                {"callers", json::array()},
                {"callees", {"foo"}},
                {"meta", {{"TestMetaHandler", {}}}}}},
              {"foo",
               {{"doesOverride", false},
                {"hasBody", true},
                {"isVirtual", false},
                {"overriddenBy", json::array()},
                {"overrides", json::array()},
                {"callers", {"main"}},
                {"callees", json::array()},
                {"meta", {{"TestMetaHandler", {}}}}}}};

  auto &mcgm = metacg::graph::MCGManager::get();
  mcgm.resetManager();
  mcgm.addToManagedGraphs("emptyGraph",std::make_unique<metacg::Callgraph>());
  mcgm.addMetaHandler<TestHandler>();

  metacg::io::JsonSource js(j);
  metacg::io::VersionTwoMetaCGReader mcgReader(js);
  mcgReader.read(mcgm);

  auto graph = *mcgm.getCallgraph();
  EXPECT_EQ(graph.size(), 2);

  const auto mainNode = graph.getMain();
  EXPECT_EQ(mainNode->getFunctionName(), "main");
  EXPECT_EQ(mainNode->getChildNodes().size(), 1);
  EXPECT_EQ(mainNode->getParentNodes().size(), 0);

  // XXX This is ugly, but we know the type of the meta handler, so we cast it.
  auto handlers = mcgm.getMetaHandlers();
  auto th = handlers.front();
  TestHandler *tmh = dynamic_cast<TestHandler *>(th);
  EXPECT_EQ(tmh->i, 2);  // We have two nodes with meta information for this handler's key
}

TEST(VersionTwoMetaCGReaderTest, TwoNodesTwoMetaDataHandler) {
  metacg::loggerutil::getLogger();

  json j;
  j["_MetaCG"] = {{"version", "1.0"}, {"generator", {{"name", "testGen"}, {"version", "1.0"}}}};
  j["_CG"] = {{"main",
               {{"doesOverride", false},
                {"hasBody", true},
                {"isVirtual", false},
                {"overriddenBy", json::array()},
                {"overrides", json::array()},
                {"callers", json::array()},
                {"callees", {"foo"}},
                {"meta", {{"TestMetaHandler", {}}}}}},
              {"foo",
               {{"doesOverride", false},
                {"hasBody", true},
                {"isVirtual", false},
                {"overriddenBy", json::array()},
                {"overrides", json::array()},
                {"callers", {"main"}},
                {"callees", json::array()},
                {"meta", {{"TestMetaHandlerOne", {}}}}}}};
  // Only used / required in this test.
  struct TestHandlerOne : public metacg::io::retriever::MetaDataHandler {
    int i{0};
    const std::string toolName() const override { return "TestMetaHandlerOne"; }
    void read([[maybe_unused]] const json &j, const std::string &functionName) override { i++; }
    bool handles(const CgNodePtr n) const override { return false; }
    json value(const CgNodePtr n) const {
      json j;
      j = 1;
      return j;
    }
  };

  auto &mcgm = metacg::graph::MCGManager::get();
  mcgm.resetManager();
  mcgm.addToManagedGraphs("emptyGraph",std::make_unique<metacg::Callgraph>());
  mcgm.addMetaHandler<TestHandler>();
  mcgm.addMetaHandler<TestHandlerOne>();

  metacg::io::JsonSource js(j);
  metacg::io::VersionTwoMetaCGReader mcgReader(js);
  mcgReader.read(mcgm);

  auto graph = *mcgm.getCallgraph();
  EXPECT_EQ(graph.size(), 2);

  const auto mainNode = graph.getMain();
  EXPECT_EQ(mainNode->getFunctionName(), "main");
  EXPECT_EQ(mainNode->getChildNodes().size(), 1);
  EXPECT_EQ(mainNode->getParentNodes().size(), 0);

  // XXX This is ugly, but we know the type of the meta handler, so we cast it.
  auto handlers = mcgm.getMetaHandlers();
  auto th = handlers[0];
  TestHandler *tmh = dynamic_cast<TestHandler *>(th);
  ASSERT_NE(tmh, nullptr);
  EXPECT_EQ(tmh->i, 1);  // We have two nodes with meta information for one handler each

  auto tho = handlers[1];
  TestHandlerOne *tmho = dynamic_cast<TestHandlerOne *>(tho);
  ASSERT_NE(tmho, nullptr);
  EXPECT_EQ(tmho->i, 1);  // We have two nodes with meta information for one handler each
}

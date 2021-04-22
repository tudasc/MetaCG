/**
 * File: MCGReaderTest.cpp
 * License: Part of the MetaCG project. Licensed under BSD 3 clause license. See LICENSE.txt file at https://github.com/tudasc/metacg/LICENSE.txt
 */

#include "gtest/gtest.h"

#include "libIPCG/MCGReader.h"
#include "LoggerUtil.h"
#include "nlohmann/json.hpp"

using json = nlohmann::json;

/**
 * MetaDataHandler used for testing
 */
  struct TestHandler : public MetaCG::io::retriever::MetaDataHandler {
    int i{0};
    const std::string toolName() const override { return "TestMetaHandler"; }
    void read([[maybe_unused]] const json &j, const std::string &functionName) override {i++;}
    bool handles(const CgNodePtr n) const override { return false; }
    int value(const CgNodePtr n) const { return i; }
  };

TEST(VersionOneMCGReaderTest, EmptyJSON) {
  json j;
  loggerutil::getLogger();

  auto &cgm = CallgraphManager::get();
  cgm.clear();
  Config c;

  MetaCG::io::JsonSource js(j);
  MetaCG::io::VersionOneMetaCGReader mcgReader(js);
  mcgReader.read(cgm);

  Callgraph& graph = cgm.getCallgraph(&cgm);
  ASSERT_EQ(graph.size(), 0);
}

TEST(VersionOneMCGReaderTest, SimpleJSON) {
  json j;
  j["main"] = {
      {"numStatements", 42},
      {"doesOverride", false},
      {"hasBody", true},
      {"isVirtual", false},
      {"overriddenBy", json::array()},
      {"overriddenFunctions", json::array()},
      {"parents", json::array()},
      {"callees", json::array()},

  };

  auto &cgm = CallgraphManager::get();

  cgm.clear();
  Config c;

  MetaCG::io::JsonSource js(j);
  MetaCG::io::VersionOneMetaCGReader mcgReader(js);
  mcgReader.read(cgm);

  Callgraph& graph = cgm.getCallgraph(&cgm);
  EXPECT_EQ(graph.size(), 1);

  CgNodePtr mainNode = graph.getNode("main");
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
      {"numStatements", 1},
      {"doesOverride", false},
      {"hasBody", true},
      {"isVirtual", false},
      {"overriddenBy", json::array()},
      {"overriddenFunctions", json::array()},
      {"parents", {"main"}},
      {"callees", json::array()},
  };

  auto &cgm = CallgraphManager::get();

  cgm.clear();
  Config c;

  MetaCG::io::JsonSource js(j);
  MetaCG::io::VersionOneMetaCGReader mcgReader(js);
  mcgReader.read(cgm);

  Callgraph& graph = cgm.getCallgraph(&cgm);
  EXPECT_EQ(graph.size(), 2);

  CgNodePtr mainNode = graph.getNode("main");
  ASSERT_NE(mainNode, nullptr);

  EXPECT_EQ(mainNode->getFunctionName(), "main");
  EXPECT_EQ(mainNode->get<pira::PiraOneData>()->getNumberOfStatements(), 42);
  EXPECT_EQ(mainNode->get<pira::PiraOneData>()->getHasBody(), true);
  EXPECT_EQ(mainNode->get<LoadImbalance::LIMetaData>()->isVirtual(), false);
  EXPECT_EQ(mainNode->getParentNodes().size(), 0);
  EXPECT_EQ(mainNode->getChildNodes().size(), 1);
  for (const auto cn : mainNode->getChildNodes()) {
    EXPECT_EQ(cn->getFunctionName(), "foo");
  }
  
  CgNodePtr fooNode = graph.getNode("foo");
  ASSERT_NE(fooNode, nullptr);

  EXPECT_EQ(fooNode->getFunctionName(), "foo");
  EXPECT_EQ(fooNode->get<pira::PiraOneData>()->getNumberOfStatements(), 1);
  EXPECT_EQ(fooNode->get<pira::PiraOneData>()->getHasBody(), true);
  EXPECT_EQ(fooNode->get<LoadImbalance::LIMetaData>()->isVirtual(), false);
  EXPECT_EQ(fooNode->getChildNodes().size(), 0);
  EXPECT_EQ(fooNode->getParentNodes().size(), 1);
  for (const auto pn : fooNode->getParentNodes()) {
    EXPECT_EQ(pn->getFunctionName(), "main");
  }
}

TEST(VersionTwoMetaCGReaderTest, EmptyJSON) {
  json j;
  loggerutil::getLogger();

  auto &cgm = CallgraphManager::get();
  cgm.clear();
  Config c;

  // No MetaData Reader added to CGManager

  MetaCG::io::JsonSource js(j);
  MetaCG::io::VersionTwoMetaCGReader mcgReader(js);
  ASSERT_THROW(mcgReader.read(cgm), std::runtime_error);
}

TEST(VersionTwoMetaCGReaderTest, EmptyCG) {
  loggerutil::getLogger();

  json j;
  j["_MetaCG"] = {
    {"version", "1.0"},
    {"generator", {{"name", "testGen"}, {"version", "1.0"}}}
  };
  j["_CG"] = {};

  auto &cgm = CallgraphManager::get();
  cgm.clear();
  Config c;

  // No MetaData Reader added to CGManager

  MetaCG::io::JsonSource js(j);
  MetaCG::io::VersionTwoMetaCGReader mcgReader(js);
  ASSERT_THROW(mcgReader.read(cgm), std::runtime_error);
}

TEST(VersionTwoMetaCGReaderTest, SingleMetaDataHandlerEmptyJSON) {
  json j;
  loggerutil::getLogger();

  auto &cgm = CallgraphManager::get();
  cgm.clear();

  cgm.addMetaHandler<TestHandler>();
  Config c;

  MetaCG::io::JsonSource js(j);
  MetaCG::io::VersionTwoMetaCGReader mcgReader(js);
  ASSERT_THROW(mcgReader.read(cgm), std::runtime_error);
}

TEST(VersionTwoMetaCGReaderTest, OneNodeNoMetaDataHandler) {
  loggerutil::getLogger();

  json j;
  j["_MetaCG"] = {
    {"version", "1.0"},
    {"generator", {{"name", "testGen"}, {"version", "1.0"}}}
  };
  j["_CG"] = {
    {"main", {
        {"doesOverride", false},
        {"hasBody", true},
        {"isVirtual", false},
        {"overriddenBy", json::array()},
        {"overrides", json::array()},
        {"callers", json::array()}, 
        {"callees", json::array()}
      }
    }
  };

  auto &cgm = CallgraphManager::get();
  cgm.clear();
  Config c;

  // No MetaData Reader added to CGManager

  MetaCG::io::JsonSource js(j);
  MetaCG::io::VersionTwoMetaCGReader mcgReader(js);
  mcgReader.read(cgm);

  auto graph = cgm.getCallgraph(&cgm);
  EXPECT_EQ(graph.size(), 1);

  const auto mainNode = graph.findMain();
  EXPECT_EQ(mainNode->getFunctionName(), "main");
  EXPECT_EQ(mainNode->getChildNodes().size(), 0);
  EXPECT_EQ(mainNode->getParentNodes().size(), 0);
}

TEST(VersionTwoMetaCGReaderTest, TwoNodesNoMetaDataHandler) {
  loggerutil::getLogger();

  json j;
  j["_MetaCG"] = {
    {"version", "1.0"},
    {"generator", {{"name", "testGen"}, {"version", "1.0"}}}
  };
  j["_CG"] = {
    {"main", {
        {"doesOverride", false},
        {"hasBody", true},
        {"isVirtual", false},
        {"overriddenBy", json::array()},
        {"overrides", json::array()},
        {"callers", json::array()}, 
        {"callees", {"foo"}},
        {"meta", {}}
      }
    },
    {"foo", {
        {"doesOverride", false},
        {"hasBody", true},
        {"isVirtual", false},
        {"overriddenBy", json::array()},
        {"overrides", json::array()},
        {"callers", {"main"}}, 
        {"callees", json::array()},
        {"meta", {}}
      }
    }
  };

  auto &cgm = CallgraphManager::get();
  cgm.clear();
  Config c;

  // No MetaData Reader added to CGManager

  MetaCG::io::JsonSource js(j);
  MetaCG::io::VersionTwoMetaCGReader mcgReader(js);
  mcgReader.read(cgm);

  auto graph = cgm.getCallgraph(&cgm);
  EXPECT_EQ(graph.size(), 2);

  const auto mainNode = graph.findMain();
  EXPECT_EQ(mainNode->getFunctionName(), "main");
  EXPECT_EQ(mainNode->getChildNodes().size(), 1);
  EXPECT_EQ(mainNode->getParentNodes().size(), 0);
}

TEST(VersionTwoMetaCGReaderTest, TwoNodesOneMetaDataHandler) {
  loggerutil::getLogger();

  json j;
  j["_MetaCG"] = {
    {"version", "1.0"},
    {"generator", {{"name", "testGen"}, {"version", "1.0"}}}
  };
  j["_CG"] = {
    {"main", {
        {"doesOverride", false},
        {"hasBody", true},
        {"isVirtual", false},
        {"overriddenBy", json::array()},
        {"overrides", json::array()},
        {"callers", json::array()}, 
        {"callees", {"foo"}},
        {"meta", { {"TestMetaHandler", {}}}}
      }
    },
    {"foo", {
        {"doesOverride", false},
        {"hasBody", true},
        {"isVirtual", false},
        {"overriddenBy", json::array()},
        {"overrides", json::array()},
        {"callers", {"main"}}, 
        {"callees", json::array()},
        {"meta", { {"TestMetaHandler", {}}}}
      }
    }
  };

  auto &cgm = CallgraphManager::get();
  cgm.clear();
  Config c;

  cgm.addMetaHandler<TestHandler>();

  MetaCG::io::JsonSource js(j);
  MetaCG::io::VersionTwoMetaCGReader mcgReader(js);
  mcgReader.read(cgm);

  auto graph = cgm.getCallgraph(&cgm);
  EXPECT_EQ(graph.size(), 2);

  const auto mainNode = graph.findMain();
  EXPECT_EQ(mainNode->getFunctionName(), "main");
  EXPECT_EQ(mainNode->getChildNodes().size(), 1);
  EXPECT_EQ(mainNode->getParentNodes().size(), 0);

  // XXX This is ugly, but we know the type of the meta handler, so we cast it.
  auto handlers = cgm.getMetaHandlers();
  auto th = handlers.front();
  TestHandler *tmh = dynamic_cast<TestHandler*>(th);
  EXPECT_EQ(tmh->i, 2); // We have two nodes with meta information for this handler's key
}

TEST(VersionTwoMetaCGReaderTest, TwoNodesTwoMetaDataHandler) {
  loggerutil::getLogger();

  json j;
  j["_MetaCG"] = {
    {"version", "1.0"},
    {"generator", {{"name", "testGen"}, {"version", "1.0"}}}
  };
  j["_CG"] = {
    {"main", {
        {"doesOverride", false},
        {"hasBody", true},
        {"isVirtual", false},
        {"overriddenBy", json::array()},
        {"overrides", json::array()},
        {"callers", json::array()}, 
        {"callees", {"foo"}},
        {"meta", { {"TestMetaHandler", {}}}}
      }
    },
    {"foo", {
        {"doesOverride", false},
        {"hasBody", true},
        {"isVirtual", false},
        {"overriddenBy", json::array()},
        {"overrides", json::array()},
        {"callers", {"main"}}, 
        {"callees", json::array()},
        {"meta", { {"TestMetaHandlerOne", {}}}}
      }
    }
  };

  auto &cgm = CallgraphManager::get();
  cgm.clear();
  Config c;

  // Only used / required in this test.
  struct TestHandlerOne : public MetaCG::io::retriever::MetaDataHandler {
    int i{0};
    const std::string toolName() const override { return "TestMetaHandlerOne"; }
    void read([[maybe_unused]] const json &j, const std::string &functionName) override {i++;}
    bool handles(const CgNodePtr n) const override { return false; }
    int value(const CgNodePtr n) const { return i; }
  };

  cgm.addMetaHandler<TestHandler>();
  cgm.addMetaHandler<TestHandlerOne>();

  MetaCG::io::JsonSource js(j);
  MetaCG::io::VersionTwoMetaCGReader mcgReader(js);
  mcgReader.read(cgm);

  auto graph = cgm.getCallgraph(&cgm);
  EXPECT_EQ(graph.size(), 2);

  const auto mainNode = graph.findMain();
  EXPECT_EQ(mainNode->getFunctionName(), "main");
  EXPECT_EQ(mainNode->getChildNodes().size(), 1);
  EXPECT_EQ(mainNode->getParentNodes().size(), 0);

  // XXX This is ugly, but we know the type of the meta handler, so we cast it.
  auto handlers = cgm.getMetaHandlers();
  auto th = handlers[0];
  TestHandler *tmh = dynamic_cast<TestHandler*>(th);
  ASSERT_NE(tmh, nullptr);
  EXPECT_EQ(tmh->i, 1); // We have two nodes with meta information for one handler each

  auto tho = handlers[1];
  TestHandlerOne *tmho = dynamic_cast<TestHandlerOne*>(tho);
  ASSERT_NE(tmho, nullptr);
  EXPECT_EQ(tmho->i, 1); // We have two nodes with meta information for one handler each
}

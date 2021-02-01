#include "gtest/gtest.h"

#include "libIPCG/IPCGReader.h"
#include "nlohmann/json.hpp"

using json = nlohmann::json;

TEST(IPCGReaderTest, EmptyJSON) {
  json j;

  auto &cgm = CallgraphManager::get();
  cgm.clear();
  Config c;

  IPCGAnal::buildFromJSON(cgm, j, &c);

  Callgraph& graph = cgm.getCallgraph(&cgm);
  ASSERT_EQ(graph.size(), 0);
}

TEST(IPCGReaderTest, SimpleJSON) {
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

  IPCGAnal::buildFromJSON(cgm, j, &c);

  Callgraph& graph = cgm.getCallgraph(&cgm);
  EXPECT_EQ(graph.size(), 1);

  CgNodePtr mainNode = graph.getNode("main");
  ASSERT_NE(mainNode, nullptr);

  EXPECT_EQ(mainNode->getFunctionName(), "main");
  EXPECT_EQ(mainNode->get<pira::PiraOneData>()->getNumberOfStatements(), 42);
  EXPECT_EQ(mainNode->get<pira::PiraOneData>()->getHasBody(), true);
  EXPECT_EQ(mainNode->get<LoadImbalance::LIMetaData>()->isVirtual(), false);
}
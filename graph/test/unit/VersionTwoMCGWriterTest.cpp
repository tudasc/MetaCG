/**
 * File: VersionTwoMCGWriterTest.cpp
 * License: Part of the MetaCG project. Licensed under BSD 3 clause license. See LICENSE.txt file at
 * https://github.com/tudasc/metacg/LICENSE.txt
 */

#include "LoggerUtil.h"
#include "gtest/gtest.h"

#include "MCGBaseInfo.h"
#include "MCGManager.h"
#include "io/VersionTwoMCGWriter.h"

class V2MCGWriterTest : public ::testing::Test {
 protected:
  void SetUp() override {
    metacg::loggerutil::getLogger();
    auto &mcgm = metacg::graph::MCGManager::get();
    mcgm.resetManager();
    mcgm.addToManagedGraphs("emptyGraph", std::make_unique<metacg::Callgraph>());
  }
};

class TestMetaData : public metacg::MetaData::Registrar<TestMetaData> {
 public:
  static constexpr const char *key = "TestMetaData";
  TestMetaData() = default;
  explicit TestMetaData(const nlohmann::json &j) {
    metadataString = j.at("metadataString");
    metadataInt = j.at("metadataInt");
    metadataFloat = j.at("metadataFloat");
  }

  nlohmann::json to_json() const final {
    nlohmann::json j;
    j["metadataString"] = metadataString;
    j["metadataInt"] = metadataInt;
    j["metadataFloat"] = metadataFloat;
    return j;
  };

  virtual const char *getKey() const final { return key; }

  std::string metadataString;
  int metadataInt = 0;
  float metadataFloat = 0.0f;
};

TEST_F(V2MCGWriterTest, DifferentMetaInformation) {
  std::string generatorName = "Test";
  metacg::MCGFileInfo mcgFileInfo = {{2, 0}, {generatorName, 0, 1, "TestSha"}};
  auto &mcgm = metacg::graph::MCGManager::get();
  metacg::io::VersionTwoMCGWriter mcgWriter(mcgFileInfo);
  metacg::io::JsonSink jsonSink;
  mcgWriter.write(jsonSink);
  EXPECT_EQ(jsonSink.getJson().dump(),
            "{\"_CG\":null,\"_MetaCG\":{\"generator\":{\"name\":\"Test\",\"sha\":\"TestSha\",\"version\":\"0.1\"},"
            "\"version\":\"2.0\"}}");
}

TEST_F(V2MCGWriterTest, OneNodeCGWrite) {
  auto &mcgm = metacg::graph::MCGManager::get();
  const auto &cg = mcgm.getCallgraph();
  cg->insert("main");
  cg->getMain()->setIsVirtual(false);
  cg->getMain()->setHasBody(true);

  std::string generatorName = "Test";
  metacg::MCGFileInfo mcgFileInfo = {{2, 0}, {generatorName, 0, 1, "TestSha"}};
  metacg::io::VersionTwoMCGWriter mcgWriter(mcgFileInfo);
  metacg::io::JsonSink jsonSink;
  mcgWriter.write(jsonSink);

  EXPECT_EQ(jsonSink.getJson().dump(),
            "{\"_CG\":{\"main\":{\"callees\":[],\"callers\":[],\"doesOverride\":false,\"hasBody\":true,\"isVirtual\":"
            "false,\"meta\":null,\"overriddenBy\":[],\"overrides\":[]}},\"_MetaCG\":{\"generator\":{\"name\":\"Test\","
            "\"sha\":\"TestSha\",\"version\":\"0.1\"},\"version\":\"2.0\"}}");
}

TEST_F(V2MCGWriterTest, TwoNodeCGWrite) {
  auto &mcgm = metacg::graph::MCGManager::get();
  const auto &cg = mcgm.getCallgraph();
  cg->insert("main");
  cg->getMain()->setIsVirtual(false);
  cg->getMain()->setHasBody(true);

  cg->insert("foo");
  cg->getNode("foo")->setIsVirtual(true);
  cg->getNode("foo")->setHasBody(true);

  std::string generatorName = "Test";
  metacg::MCGFileInfo mcgFileInfo = {{2, 0}, {generatorName, 0, 1, "TestSha"}};
  metacg::io::VersionTwoMCGWriter mcgWriter(mcgFileInfo);
  metacg::io::JsonSink jsonSink;
  mcgWriter.write(jsonSink);
  EXPECT_EQ(
      jsonSink.getJson().dump(),
      "{\"_CG\":{\"foo\":{\"callees\":[],\"callers\":[],\"doesOverride\":false,\"hasBody\":true,\"isVirtual\":true,"
      "\"meta\":null,\"overriddenBy\":[],\"overrides\":[]},\"main\":{\"callees\":[],\"callers\":[],\"doesOverride\":"
      "false,\"hasBody\":true,\"isVirtual\":false,\"meta\":null,\"overriddenBy\":[],\"overrides\":[]}},\"_MetaCG\":{"
      "\"generator\":{\"name\":\"Test\",\"sha\":\"TestSha\",\"version\":\"0.1\"},\"version\":\"2.0\"}}");
}

TEST_F(V2MCGWriterTest, TwoNodeOneEdgeCGWrite) {
  auto &mcgm = metacg::graph::MCGManager::get();
  const auto &cg = mcgm.getCallgraph();
  cg->insert("main");
  cg->getMain()->setIsVirtual(false);
  cg->getMain()->setHasBody(true);

  cg->insert("foo");
  cg->getNode("foo")->setIsVirtual(true);
  cg->getNode("foo")->setHasBody(true);

  cg->addEdge("main", "foo");

  std::string generatorName = "Test";
  metacg::MCGFileInfo mcgFileInfo = {{2, 0}, {generatorName, 0, 1, "TestSha"}};
  metacg::io::VersionTwoMCGWriter mcgWriter(mcgFileInfo);
  metacg::io::JsonSink jsonSink;
  mcgWriter.write(jsonSink);

  EXPECT_EQ(
      jsonSink.getJson().dump(),
      "{\"_CG\":{\"foo\":{\"callees\":[],\"callers\":[\"main\"],\"doesOverride\":false,\"hasBody\":true,\"isVirtual\":"
      "true,\"meta\":null,\"overriddenBy\":[],\"overrides\":[]},\"main\":{\"callees\":[\"foo\"],\"callers\":[],"
      "\"doesOverride\":false,\"hasBody\":true,\"isVirtual\":false,\"meta\":null,\"overriddenBy\":[],\"overrides\":[]}}"
      ",\"_MetaCG\":{\"generator\":{\"name\":\"Test\",\"sha\":\"TestSha\",\"version\":\"0.1\"},\"version\":\"2.0\"}}");
}

TEST_F(V2MCGWriterTest, ThreeNodeOneEdgeCGWrite) {
  auto &mcgm = metacg::graph::MCGManager::get();
  const auto &cg = mcgm.getCallgraph();
  cg->insert("main");
  cg->getMain()->setIsVirtual(false);
  cg->getMain()->setHasBody(true);

  cg->insert("foo");
  cg->getNode("foo")->setIsVirtual(true);
  cg->getNode("foo")->setHasBody(true);

  cg->insert("bar");
  cg->getNode("bar")->setIsVirtual(false);
  cg->getNode("bar")->setHasBody(false);

  cg->addEdge("main", "foo");

  std::string generatorName = "Test";
  metacg::MCGFileInfo mcgFileInfo = {{2, 0}, {generatorName, 0, 1, "TestSha"}};
  metacg::io::VersionTwoMCGWriter mcgWriter(mcgFileInfo);
  metacg::io::JsonSink jsonSink;
  mcgWriter.write(jsonSink);

  EXPECT_EQ(jsonSink.getJson().dump(),
            "{\"_CG\":{\"bar\":{\"callees\":[],\"callers\":[],\"doesOverride\":false,\"hasBody\":false,\"isVirtual\":"
            "false,\"meta\":null,\"overriddenBy\":[],\"overrides\":[]},\"foo\":{\"callees\":[],\"callers\":[\"main\"],"
            "\"doesOverride\":false,\"hasBody\":true,\"isVirtual\":true,\"meta\":null,\"overriddenBy\":[],"
            "\"overrides\":[]},\"main\":{\"callees\":[\"foo\"],\"callers\":[],\"doesOverride\":false,\"hasBody\":true,"
            "\"isVirtual\":false,\"meta\":null,\"overriddenBy\":[],\"overrides\":[]}},\"_MetaCG\":{\"generator\":{"
            "\"name\":\"Test\",\"sha\":\"TestSha\",\"version\":\"0.1\"},\"version\":\"2.0\"}}");
}

TEST_F(V2MCGWriterTest, MetadataCGWrite) {
  auto &mcgm = metacg::graph::MCGManager::get();
  const auto &cg = mcgm.getCallgraph();
  cg->insert("main");
  cg->getMain()->setIsVirtual(false);
  cg->getMain()->setHasBody(true);
  // metadata does not need to be freed,
  // it is now owned by the node
  auto testMetaData = new TestMetaData();
  cg->getMain()->addMetaData(testMetaData);

  std::string generatorName = "Test";
  metacg::MCGFileInfo mcgFileInfo = {{2, 0}, {generatorName, 0, 1, "TestSha"}};
  metacg::io::VersionTwoMCGWriter mcgWriter(mcgFileInfo);
  metacg::io::JsonSink jsonSink;
  mcgWriter.write(jsonSink);

  EXPECT_EQ(jsonSink.getJson().dump(),
            "{\"_CG\":{\"main\":{\"callees\":[],\"callers\":[],\"doesOverride\":false,\"hasBody\":true,\"isVirtual\":"
            "false,\"meta\":{\"TestMetaData\":{\"metadataFloat\":0.0,\"metadataInt\":0,\"metadataString\":\"\"}},"
            "\"overriddenBy\":[],\"overrides\":[]}},\"_MetaCG\":{\"generator\":{\"name\":\"Test\",\"sha\":\"TestSha\","
            "\"version\":\"0.1\"},\"version\":\"2.0\"}}");
}

TEST_F(V2MCGWriterTest, WriteByName) {
  auto &mcgm = metacg::graph::MCGManager::get();
  mcgm.addToManagedGraphs("newGraph", std::make_unique<metacg::Callgraph>());
  const auto &cg = mcgm.getCallgraph();
  EXPECT_EQ(mcgm.graphs_size(),2);
  EXPECT_EQ(mcgm.getActiveGraphName(),"newGraph");
  cg->insert("main");
  cg->getMain()->setIsVirtual(false);
  cg->getMain()->setHasBody(true);
  // metadata does not need to be freed,
  // it is now owned by the node
  auto testMetaData = new TestMetaData();
  cg->getMain()->addMetaData(testMetaData);

  std::string generatorName = "Test";
  metacg::MCGFileInfo mcgFileInfo = {{2, 0}, {generatorName, 0, 1, "TestSha"}};
  metacg::io::VersionTwoMCGWriter mcgWriter(mcgFileInfo);
  metacg::io::JsonSink jsonSink;
  mcgWriter.write("newGraph",jsonSink);
  EXPECT_EQ(jsonSink.getJson().dump(),
            "{\"_CG\":{\"main\":{\"callees\":[],\"callers\":[],\"doesOverride\":false,\"hasBody\":true,\"isVirtual\":"
            "false,\"meta\":{\"TestMetaData\":{\"metadataFloat\":0.0,\"metadataInt\":0,\"metadataString\":\"\"}},"
            "\"overriddenBy\":[],\"overrides\":[]}},\"_MetaCG\":{\"generator\":{\"name\":\"Test\",\"sha\":\"TestSha\","
            "\"version\":\"0.1\"},\"version\":\"2.0\"}}");

  mcgWriter.write("emptyGraph",jsonSink);
  EXPECT_EQ(jsonSink.getJson().dump(),
            "{\"_CG\":null,"
            "\"_MetaCG\":{\"generator\":{\"name\":\"Test\",\"sha\":\"TestSha\","
            "\"version\":\"0.1\"},\"version\":\"2.0\"}}");
  mcgm.resetManager();
}

TEST_F(V2MCGWriterTest, WritePointer) {
  auto &mcgm = metacg::graph::MCGManager::get();
  mcgm.addToManagedGraphs("newGraph", std::make_unique<metacg::Callgraph>());
  const auto &cg = mcgm.getCallgraph();
  EXPECT_EQ(mcgm.graphs_size(),2);
  EXPECT_EQ(mcgm.getActiveGraphName(),"newGraph");
  cg->insert("main");
  cg->getMain()->setIsVirtual(false);
  cg->getMain()->setHasBody(true);
  // metadata does not need to be freed,
  // it is now owned by the node
  auto testMetaData = new TestMetaData();
  cg->getMain()->addMetaData(testMetaData);

  std::string generatorName = "Test";
  metacg::MCGFileInfo mcgFileInfo = {{2, 0}, {generatorName, 0, 1, "TestSha"}};
  metacg::io::VersionTwoMCGWriter mcgWriter(mcgFileInfo);
  metacg::io::JsonSink jsonSink;
  mcgWriter.write(mcgm.getCallgraph("newGraph"),jsonSink);
  EXPECT_EQ(jsonSink.getJson().dump(),
            "{\"_CG\":{\"main\":{\"callees\":[],\"callers\":[],\"doesOverride\":false,\"hasBody\":true,\"isVirtual\":"
            "false,\"meta\":{\"TestMetaData\":{\"metadataFloat\":0.0,\"metadataInt\":0,\"metadataString\":\"\"}},"
            "\"overriddenBy\":[],\"overrides\":[]}},\"_MetaCG\":{\"generator\":{\"name\":\"Test\",\"sha\":\"TestSha\","
            "\"version\":\"0.1\"},\"version\":\"2.0\"}}");

  mcgWriter.write(mcgm.getCallgraph("emptyGraph"),jsonSink);
  EXPECT_EQ(jsonSink.getJson().dump(),
            "{\"_CG\":null,"
            "\"_MetaCG\":{\"generator\":{\"name\":\"Test\",\"sha\":\"TestSha\","
            "\"version\":\"0.1\"},\"version\":\"2.0\"}}");
  mcgm.resetManager();
}

TEST_F(V2MCGWriterTest, SwitchBeforeWrite) {
  auto &mcgm = metacg::graph::MCGManager::get();
  mcgm.addToManagedGraphs("newGraph", std::make_unique<metacg::Callgraph>());
  const auto &cg = mcgm.getCallgraph();
  EXPECT_EQ(mcgm.graphs_size(),2);
  EXPECT_EQ(mcgm.getActiveGraphName(),"newGraph");
  cg->insert("main");
  cg->getMain()->setIsVirtual(false);
  cg->getMain()->setHasBody(true);
  // metadata does not need to be freed,
  // it is now owned by the node
  auto testMetaData = new TestMetaData();
  cg->getMain()->addMetaData(testMetaData);

  std::string generatorName = "Test";
  metacg::MCGFileInfo mcgFileInfo = {{2, 0}, {generatorName, 0, 1, "TestSha"}};
  metacg::io::VersionTwoMCGWriter mcgWriter(mcgFileInfo);
  metacg::io::JsonSink jsonSink;

  mcgm.setActive("newGraph");
  mcgm.setActive("emptyGraph");
  mcgm.setActive("newGraph");

  mcgWriter.write(jsonSink);
  EXPECT_EQ(jsonSink.getJson().dump(),
            "{\"_CG\":{\"main\":{\"callees\":[],\"callers\":[],\"doesOverride\":false,\"hasBody\":true,\"isVirtual\":"
            "false,\"meta\":{\"TestMetaData\":{\"metadataFloat\":0.0,\"metadataInt\":0,\"metadataString\":\"\"}},"
            "\"overriddenBy\":[],\"overrides\":[]}},\"_MetaCG\":{\"generator\":{\"name\":\"Test\",\"sha\":\"TestSha\","
            "\"version\":\"0.1\"},\"version\":\"2.0\"}}");

  mcgm.setActive("emptyGraph");
  mcgWriter.write(jsonSink);
  EXPECT_EQ(jsonSink.getJson().dump(),
            "{\"_CG\":null,"
            "\"_MetaCG\":{\"generator\":{\"name\":\"Test\",\"sha\":\"TestSha\","
            "\"version\":\"0.1\"},\"version\":\"2.0\"}}");
  mcgm.resetManager();
}

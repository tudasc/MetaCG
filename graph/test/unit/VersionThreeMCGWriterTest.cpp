/**
 * File: VersionThreeMCGWriterTest.cpp
 * License: Part of the metacg project. Licensed under BSD 3 clause license. See LICENSE.txt file at
 * https://github.com/tudasc/metacg/LICENSE.txt
 */

#include "LoggerUtil.h"
#include "gtest/gtest.h"

#include "MCGBaseInfo.h"
#include "MCGManager.h"
#include "io/VersionThreeMCGWriter.h"

class V3MCGWriterTest : public ::testing::Test {
 protected:
  void SetUp() override {
    metacg::loggerutil::getLogger();
    auto& mcgm = metacg::graph::MCGManager::get();
    mcgm.resetManager();
    mcgm.addToManagedGraphs("emptyGraph", std::make_unique<metacg::Callgraph>());
  }
};

class TestMetaData : public metacg::MetaData::Registrar<TestMetaData> {
 public:
  static constexpr const char* key = "TestMetaData";
  TestMetaData() = default;
  explicit TestMetaData(const nlohmann::json& j) {
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

  const char* getKey() const final { return key; }

  std::string metadataString;
  int metadataInt = 0;
  float metadataFloat = 0.0f;
};

TEST_F(V3MCGWriterTest, DifferentMetaInformation) {
  const std::string generatorName = "Test";
  const metacg::MCGFileInfo mcgFileInfo = {{3, 0}, {generatorName, 0, 1, "TestSha"}};
  metacg::io::VersionThreeMCGWriter mcgWriter(mcgFileInfo);
  metacg::io::JsonSink jsonSink;
  mcgWriter.write(jsonSink);
  EXPECT_EQ(jsonSink.getJson().dump(),
            "{\"_CG\":{\"edges\":[],\"nodes\":[]},\"_MetaCG\":{\"generator\":{\"name\":\"Test\","
            "\"sha\":\"TestSha\",\"version\":\"0.1\"},\"version\":\"3.0\"}}");
}

TEST_F(V3MCGWriterTest, DifferentMetaInformationDebug) {
  const std::string generatorName = "Test";
  const metacg::MCGFileInfo mcgFileInfo = {{3, 0}, {generatorName, 0, 1, "TestSha"}};
  metacg::io::VersionThreeMCGWriter mcgWriter(mcgFileInfo, true);
  metacg::io::JsonSink jsonSink;
  mcgWriter.write(jsonSink);
  EXPECT_EQ(jsonSink.getJson().dump(),
            "{\"_CG\":{\"edges\":[],\"nodes\":[]},\"_MetaCG\":{\"generator\":{\"name\":\"Test\","
            "\"sha\":\"TestSha\",\"version\":\"0.1\"},\"version\":\"3.0\"}}");
}

TEST_F(V3MCGWriterTest, OneNodeCGWrite) {
  auto& mcgm = metacg::graph::MCGManager::get();
  const auto& cg = mcgm.getCallgraph();
  cg->insert("main", "main.cpp");
  cg->getMain()->setHasBody(true);

  const std::string generatorName = "Test";
  const metacg::MCGFileInfo mcgFileInfo = {{3, 0}, {generatorName, 0, 1, "TestSha"}};
  metacg::io::VersionThreeMCGWriter mcgWriter(mcgFileInfo);
  metacg::io::JsonSink jsonSink;
  mcgWriter.write(jsonSink);

  EXPECT_EQ(jsonSink.getJson().dump(),
            "{\"_CG\":{\"edges\":[],\"nodes\":["
            "[13570296075836900426,{\"functionName\":\"main\",\"hasBody\":true,\"meta\":null,\"origin\":\"main.cpp\"}]"
            "]},\"_MetaCG\":{\"generator\":{\"name\":\"Test\",\"sha\":\"TestSha\",\"version\":\"0.1\"},\"version\":\"3."
            "0\"}}");
}

TEST_F(V3MCGWriterTest, OneNodeCGWriteDebug) {
  auto& mcgm = metacg::graph::MCGManager::get();
  const auto& cg = mcgm.getCallgraph();
  cg->insert("main", "main.cpp");
  cg->getMain()->setHasBody(true);

  const std::string generatorName = "Test";
  const metacg::MCGFileInfo mcgFileInfo = {{3, 0}, {generatorName, 0, 1, "TestSha"}};
  metacg::io::VersionThreeMCGWriter mcgWriter(mcgFileInfo, true);

  metacg::io::JsonSink jsonSink;
  mcgWriter.write(jsonSink);

  EXPECT_EQ(jsonSink.getJson().dump(),
            "{\"_CG\":{\"edges\":[],\"nodes\":["
            "[\"main\",{\"callees\":[],\"callers\":[],\"functionName\":\"main\",\"hasBody\":true,\"meta\":null,"
            "\"origin\":\"main.cpp\"}]"
            "]},\"_MetaCG\":{\"generator\":{\"name\":\"Test\",\"sha\":\"TestSha\",\"version\":\"0.1\"},\"version\":\"3."
            "0\"}}");
}

TEST_F(V3MCGWriterTest, TwoNodeCGWrite) {
  auto& mcgm = metacg::graph::MCGManager::get();
  const auto& cg = mcgm.getCallgraph();
  cg->insert("main", "main.cpp");
  cg->getMain()->setHasBody(true);

  cg->insert("foo", "main.cpp");
  cg->getNode("foo")->setHasBody(true);

  const std::string generatorName = "Test";
  const metacg::MCGFileInfo mcgFileInfo = {{3, 0}, {generatorName, 0, 1, "TestSha"}};
  metacg::io::VersionThreeMCGWriter mcgWriter(mcgFileInfo);
  metacg::io::JsonSink jsonSink;
  mcgWriter.write(jsonSink);
  EXPECT_EQ(jsonSink.getJson().dump(),
            "{\"_CG\":{\"edges\":[],\"nodes\":["
            "[6731461454900136138,{\"functionName\":\"foo\",\"hasBody\":true,\"meta\":null,\"origin\":\"main.cpp\"}],"
            "[13570296075836900426,{\"functionName\":\"main\",\"hasBody\":true,\"meta\":null,\"origin\":\"main.cpp\"}]"
            "]},\"_MetaCG\":{\"generator\":{\"name\":\"Test\",\"sha\":\"TestSha\",\"version\":\"0.1\"},\"version\":\"3."
            "0\"}}");
}

TEST_F(V3MCGWriterTest, TwoNodeCGWriteDebug) {
  auto& mcgm = metacg::graph::MCGManager::get();
  const auto& cg = mcgm.getCallgraph();
  cg->insert("main", "main.cpp");
  cg->getMain()->setHasBody(true);

  cg->insert("foo", "main.cpp");
  cg->getNode("foo")->setHasBody(true);

  const std::string generatorName = "Test";
  const metacg::MCGFileInfo mcgFileInfo = {{3, 0}, {generatorName, 0, 1, "TestSha"}};
  metacg::io::VersionThreeMCGWriter mcgWriter(mcgFileInfo, true);
  metacg::io::JsonSink jsonSink;
  mcgWriter.write(jsonSink);
  EXPECT_EQ(jsonSink.getJson().dump(),
            "{\"_CG\":{\"edges\":[],\"nodes\":["
            "[\"foo\",{\"callees\":[],\"callers\":[],\"functionName\":\"foo\",\"hasBody\":true,\"meta\":null,"
            "\"origin\":\"main.cpp\"}],"
            "[\"main\",{\"callees\":[],\"callers\":[],\"functionName\":\"main\",\"hasBody\":true,\"meta\":null,"
            "\"origin\":\"main.cpp\"}]"
            "]},\"_MetaCG\":{\"generator\":{\"name\":\"Test\",\"sha\":\"TestSha\",\"version\":\"0.1\"},\"version\":\"3."
            "0\"}}");
}

TEST_F(V3MCGWriterTest, TwoNodeOneEdgeCGWrite) {
  auto& mcgm = metacg::graph::MCGManager::get();
  const auto& cg = mcgm.getCallgraph();
  cg->insert("main", "main.cpp");
  cg->getMain()->setHasBody(true);

  cg->insert("foo", "main.cpp");
  cg->getNode("foo")->setHasBody(true);

  cg->addEdge("main", "foo");

  const std::string generatorName = "Test";
  const metacg::MCGFileInfo mcgFileInfo = {{3, 0}, {generatorName, 0, 1, "TestSha"}};
  metacg::io::VersionThreeMCGWriter mcgWriter(mcgFileInfo);
  metacg::io::JsonSink jsonSink;
  mcgWriter.write(jsonSink);

  EXPECT_EQ(jsonSink.getJson().dump(),
            "{\"_CG\":{\"edges\":[[[13570296075836900426,6731461454900136138],null]],\"nodes\":["
            "[6731461454900136138,{\"functionName\":\"foo\",\"hasBody\":true,\"meta\":null,\"origin\":\"main.cpp\"}],"
            "[13570296075836900426,{\"functionName\":\"main\",\"hasBody\":true,\"meta\":null,\"origin\":\"main.cpp\"}]"
            "]},\"_MetaCG\":{\"generator\":{\"name\":\"Test\",\"sha\":\"TestSha\",\"version\":\"0.1\"},\"version\":\"3."
            "0\"}}");
}

TEST_F(V3MCGWriterTest, TwoNodeOneEdgeCGWriteDebug) {
  auto& mcgm = metacg::graph::MCGManager::get();
  const auto& cg = mcgm.getCallgraph();
  cg->insert("main", "main.cpp");
  cg->getMain()->setHasBody(true);

  cg->insert("foo", "main.cpp");
  cg->getNode("foo")->setHasBody(true);

  cg->addEdge("main", "foo");

  const std::string generatorName = "Test";
  const metacg::MCGFileInfo mcgFileInfo = {{3, 0}, {generatorName, 0, 1, "TestSha"}};
  metacg::io::VersionThreeMCGWriter mcgWriter(mcgFileInfo, true);
  metacg::io::JsonSink jsonSink;
  mcgWriter.write(jsonSink);

  EXPECT_EQ(jsonSink.getJson().dump(),
            "{\"_CG\":{\"edges\":[[[13570296075836900426,6731461454900136138],null]],\"nodes\":["
            "[\"foo\",{\"callees\":[],\"callers\":[\"main\"],\"functionName\":\"foo\",\"hasBody\":true,\"meta\":null,"
            "\"origin\":\"main.cpp\"}],"
            "[\"main\",{\"callees\":[\"foo\"],\"callers\":[],\"functionName\":\"main\",\"hasBody\":true,\"meta\":null,"
            "\"origin\":\"main.cpp\"}]"
            "]},\"_MetaCG\":{\"generator\":{\"name\":\"Test\",\"sha\":\"TestSha\",\"version\":\"0.1\"},\"version\":\"3."
            "0\"}}");
}

TEST_F(V3MCGWriterTest, ThreeNodeOneEdgeCGWrite) {
  auto& mcgm = metacg::graph::MCGManager::get();
  const auto& cg = mcgm.getCallgraph();
  cg->insert("main", "main.cpp");
  cg->getMain()->setHasBody(true);

  cg->insert("foo", "main.cpp");
  cg->getNode("foo")->setHasBody(true);

  cg->insert("bar", "bar.cpp");
  cg->getNode("bar")->setHasBody(false);

  cg->addEdge("main", "foo");

  const std::string generatorName = "Test";
  const metacg::MCGFileInfo mcgFileInfo = {{3, 0}, {generatorName, 0, 1, "TestSha"}};
  metacg::io::VersionThreeMCGWriter mcgWriter(mcgFileInfo);
  metacg::io::JsonSink jsonSink;
  mcgWriter.write(jsonSink);

  EXPECT_EQ(
      jsonSink.getJson().dump(),
      "{\"_CG\":{\"edges\":[[[13570296075836900426,6731461454900136138],null]],\"nodes\":[["
      "17384295136535843005,{\"functionName\":\"bar\",\"hasBody\":false,\"meta\":null,\"origin\":\"bar.cpp\"}],["
      "6731461454900136138,{\"functionName\":\"foo\",\"hasBody\":true,\"meta\":null,\"origin\":\"main.cpp\"}],["
      "13570296075836900426,{\"functionName\":\"main\",\"hasBody\":true,\"meta\":null,\"origin\":\"main.cpp\"}]]},"
      "\"_MetaCG\":{\"generator\":{\"name\":\"Test\",\"sha\":\"TestSha\",\"version\":\"0.1\"},\"version\":\"3.0\"}}");
}

TEST_F(V3MCGWriterTest, ThreeNodeOneEdgeCGWriteDebug) {
  auto& mcgm = metacg::graph::MCGManager::get();
  const auto& cg = mcgm.getCallgraph();
  cg->insert("main", "main.cpp");
  cg->getMain()->setHasBody(true);

  cg->insert("foo", "main.cpp");
  cg->getNode("foo")->setHasBody(true);

  cg->insert("bar", "bar.cpp");
  cg->getNode("bar")->setHasBody(false);

  cg->addEdge("main", "foo");

  const std::string generatorName = "Test";
  const metacg::MCGFileInfo mcgFileInfo = {{3, 0}, {generatorName, 0, 1, "TestSha"}};
  metacg::io::VersionThreeMCGWriter mcgWriter(mcgFileInfo, true);
  metacg::io::JsonSink jsonSink;
  mcgWriter.write(jsonSink);

  EXPECT_EQ(jsonSink.getJson().dump(),
            "{\"_CG\":{\"edges\":[[[13570296075836900426,6731461454900136138],null]],\"nodes\":["
            "[\"bar\",{\"callees\":[],\"callers\":[],\"functionName\":\"bar\",\"hasBody\":false,\"meta\":null,"
            "\"origin\":\"bar.cpp\"}],"
            "[\"foo\",{\"callees\":[],\"callers\":[\"main\"],\"functionName\":\"foo\",\"hasBody\":true,\"meta\":null,"
            "\"origin\":\"main.cpp\"}],"
            "[\"main\",{\"callees\":[\"foo\"],\"callers\":[],\"functionName\":\"main\",\"hasBody\":true,\"meta\":null,"
            "\"origin\":\"main.cpp\"}]"
            "]},\"_MetaCG\":{\"generator\":{\"name\":\"Test\",\"sha\":\"TestSha\",\"version\":\"0.1\"},\"version\":\"3."
            "0\"}}");
}

TEST_F(V3MCGWriterTest, GraphMetadataCGWrite) {
  auto& mcgm = metacg::graph::MCGManager::get();
  const auto& cg = mcgm.getCallgraph();
  cg->insert("main", "main.cpp");
  cg->getMain()->setHasBody(true);
  // metadata does not need to be freed,
  // it is now owned by the node
  auto testMetaData = new TestMetaData();
  testMetaData->metadataFloat = 0;
  testMetaData->metadataInt = 1337;
  testMetaData->metadataString = "TestString";
  cg->getMain()->addMetaData(testMetaData);

  const std::string generatorName = "Test";
  const metacg::MCGFileInfo mcgFileInfo = {{3, 0}, {generatorName, 0, 1, "TestSha"}};
  metacg::io::VersionThreeMCGWriter mcgWriter(mcgFileInfo);
  metacg::io::JsonSink jsonSink;
  mcgWriter.write(jsonSink);

  EXPECT_EQ(jsonSink.getJson().dump(),
            "{\"_CG\":{\"edges\":[],\"nodes\":["
            "[13570296075836900426,{\"functionName\":\"main\",\"hasBody\":true,"
            "\"meta\":{\"TestMetaData\":{\"metadataFloat\":0.0,\"metadataInt\":1337,\"metadataString\":\"TestString\"}}"
            ",\"origin\":\"main.cpp\"}]"
            "]},\"_MetaCG\":{\"generator\":{\"name\":\"Test\",\"sha\":\"TestSha\",\"version\":\"0.1\"},\"version\":\"3."
            "0\"}}");
}

TEST_F(V3MCGWriterTest, GraphMetadataCGWriteDebug) {
  auto& mcgm = metacg::graph::MCGManager::get();
  const auto& cg = mcgm.getCallgraph();
  cg->insert("main", "main.cpp");
  cg->getMain()->setHasBody(true);
  // metadata does not need to be freed,
  // it is now owned by the node
  auto testMetaData = new TestMetaData();
  testMetaData->metadataFloat = 0;
  testMetaData->metadataInt = 1337;
  testMetaData->metadataString = "TestString";

  cg->getMain()->addMetaData(testMetaData);

  const std::string generatorName = "Test";
  const metacg::MCGFileInfo mcgFileInfo = {{3, 0}, {generatorName, 0, 1, "TestSha"}};
  metacg::io::VersionThreeMCGWriter mcgWriter(mcgFileInfo, true);
  metacg::io::JsonSink jsonSink;
  mcgWriter.write(jsonSink);

  EXPECT_EQ(jsonSink.getJson().dump(),
            "{\"_CG\":{\"edges\":[],\"nodes\":["
            "[\"main\",{\"callees\":[],\"callers\":[],\"functionName\":\"main\",\"hasBody\":true,"
            "\"meta\":{\"TestMetaData\":{\"metadataFloat\":0.0,\"metadataInt\":1337,\"metadataString\":\"TestString\"}}"
            ",\"origin\":\"main.cpp\"}]"
            "]},\"_MetaCG\":{\"generator\":{\"name\":\"Test\",\"sha\":\"TestSha\",\"version\":\"0.1\"},\"version\":\"3."
            "0\"}}");
}

TEST_F(V3MCGWriterTest, EdgeMetadataCGWrite) {
  auto& mcgm = metacg::graph::MCGManager::get();
  const auto& cg = mcgm.getCallgraph();
  cg->insert("main", "main.cpp");
  cg->getMain()->setHasBody(true);
  cg->insert("foo", "main.cpp");
  cg->getNode("foo")->setHasBody(true);

  cg->addEdge("main", "foo");

  // metadata does not need to be freed,
  // it is now owned by the node
  auto testMetaData = new TestMetaData();
  testMetaData->metadataFloat = 0;
  testMetaData->metadataInt = 1337;
  testMetaData->metadataString = "TestString";

  cg->addEdgeMetaData("main", "foo", testMetaData);

  const std::string generatorName = "Test";
  const metacg::MCGFileInfo mcgFileInfo = {{3, 0}, {generatorName, 0, 1, "TestSha"}};
  metacg::io::VersionThreeMCGWriter mcgWriter(mcgFileInfo, false);
  metacg::io::JsonSink jsonSink;
  mcgWriter.write(jsonSink);
  // clang-format off
  EXPECT_EQ(jsonSink.getJson().dump(),
            "{\"_CG\":{\"edges\":["
                "["
                  "[13570296075836900426,6731461454900136138],{\"TestMetaData\":{\"metadataFloat\":0.0,\"metadataInt\":1337,\"metadataString\":\"TestString\"}}"
                "]"
              "],"
            "\"nodes\":["
              "[6731461454900136138,{\"functionName\":\"foo\",\"hasBody\":true,\"meta\":null,\"origin\":\"main.cpp\"}],"
              "[13570296075836900426,{\"functionName\":\"main\",\"hasBody\":true,\"meta\":null,\"origin\":\"main.cpp\"}]"
            "]},"
            "\"_MetaCG\":{\"generator\":{\"name\":\"Test\",\"sha\":\"TestSha\",\"version\":\"0.1\"},\"version\":\"3.0\"}}");
  // clang-format on
}

TEST_F(V3MCGWriterTest, EdgeMetadataCGWriteDebug) {
  auto& mcgm = metacg::graph::MCGManager::get();
  const auto& cg = mcgm.getCallgraph();
  cg->insert("main", "main.cpp");
  cg->getMain()->setHasBody(true);
  cg->insert("foo", "main.cpp");
  cg->getNode("foo")->setHasBody(true);

  cg->addEdge("main", "foo");

  // metadata does not need to be freed,
  // it is now owned by the node
  auto testMetaData = new TestMetaData();
  testMetaData->metadataFloat = 0;
  testMetaData->metadataInt = 1337;
  testMetaData->metadataString = "TestString";

  cg->addEdgeMetaData("main", "foo", testMetaData);

  const std::string generatorName = "Test";
  const metacg::MCGFileInfo mcgFileInfo = {{3, 0}, {generatorName, 0, 1, "TestSha"}};
  metacg::io::VersionThreeMCGWriter mcgWriter(mcgFileInfo, true);
  metacg::io::JsonSink jsonSink;
  mcgWriter.write(jsonSink);

  // clang-format off
  EXPECT_EQ(
      jsonSink.getJson().dump(),
      "{\"_CG\":{\"edges\":["
        "["
          "[13570296075836900426,6731461454900136138],{\"TestMetaData\":{\"metadataFloat\":0.0,\"metadataInt\":1337,\"metadataString\":\"TestString\"}}"
        "]"
      "],"
      "\"nodes\":["
        "[\"foo\",{\"callees\":[],\"callers\":[\"main\"],\"functionName\":\"foo\",\"hasBody\":true,\"meta\":null,\"origin\":\"main.cpp\"}],"
        "[\"main\",{\"callees\":[\"foo\"],\"callers\":[],\"functionName\":\"main\",\"hasBody\":true,\"meta\":null,\"origin\":\"main.cpp\"}]"
      "]},"
      "\"_MetaCG\":{\"generator\":{\"name\":\"Test\",\"sha\":\"TestSha\",\"version\":\"0.1\"},\"version\":\"3.0\"}}");
  // clang-format on
}

TEST_F(V3MCGWriterTest, WriteByName) {
  auto& mcgm = metacg::graph::MCGManager::get();
  mcgm.addToManagedGraphs("newGraph", std::make_unique<metacg::Callgraph>());
  const auto& cg = mcgm.getCallgraph();
  cg->insert("main", "main.cpp");
  cg->getMain()->setHasBody(true);

  const std::string generatorName = "Test";
  const metacg::MCGFileInfo mcgFileInfo = {{3, 0}, {generatorName, 0, 1, "TestSha"}};
  metacg::io::VersionThreeMCGWriter mcgWriter(mcgFileInfo);
  metacg::io::JsonSink jsonSink;
  mcgWriter.write("emptyGraph", jsonSink);
  EXPECT_EQ(jsonSink.getJson().dump(),
            "{\"_CG\":{\"edges\":[],\"nodes\":[]},"
            "\"_MetaCG\":{\"generator\":{\"name\":\"Test\",\"sha\":\"TestSha\",\"version\":\"0.1\"},\"version\":\"3."
            "0\"}}");

  mcgWriter.write("newGraph", jsonSink);
  EXPECT_EQ(jsonSink.getJson().dump(),
            "{\"_CG\":{\"edges\":[],\"nodes\":["
            "[13570296075836900426,{\"functionName\":\"main\",\"hasBody\":true,\"meta\":null,\"origin\":\"main.cpp\"}]"
            "]},\"_MetaCG\":{\"generator\":{\"name\":\"Test\",\"sha\":\"TestSha\",\"version\":\"0.1\"},\"version\":\"3."
            "0\"}}");
}

TEST_F(V3MCGWriterTest, WriteByNameDebug) {
  auto& mcgm = metacg::graph::MCGManager::get();
  mcgm.addToManagedGraphs("newGraph", std::make_unique<metacg::Callgraph>());
  const auto& cg = mcgm.getCallgraph();
  cg->insert("main", "main.cpp");
  cg->getMain()->setHasBody(true);

  const std::string generatorName = "Test";
  const metacg::MCGFileInfo mcgFileInfo = {{3, 0}, {generatorName, 0, 1, "TestSha"}};
  metacg::io::VersionThreeMCGWriter mcgWriter(mcgFileInfo, true);
  metacg::io::JsonSink jsonSink;
  mcgWriter.write("emptyGraph", jsonSink);
  EXPECT_EQ(jsonSink.getJson().dump(),
            "{\"_CG\":{\"edges\":[],\"nodes\":[]},"
            "\"_MetaCG\":{\"generator\":{\"name\":\"Test\",\"sha\":\"TestSha\",\"version\":\"0.1\"},\"version\":\"3."
            "0\"}}");

  mcgWriter.write("newGraph", jsonSink);
  EXPECT_EQ(jsonSink.getJson().dump(),
            "{\"_CG\":{\"edges\":[],\"nodes\":["
            "[\"main\",{\"callees\":[],\"callers\":[],\"functionName\":\"main\",\"hasBody\":true,\"meta\":null,"
            "\"origin\":\"main.cpp\"}]"
            "]},\"_MetaCG\":{\"generator\":{\"name\":\"Test\",\"sha\":\"TestSha\",\"version\":\"0.1\"},\"version\":\"3."
            "0\"}}");
}

TEST_F(V3MCGWriterTest, WritePointer) {
  auto& mcgm = metacg::graph::MCGManager::get();
  mcgm.addToManagedGraphs("newGraph", std::make_unique<metacg::Callgraph>());
  const auto& cg = mcgm.getCallgraph();
  cg->insert("main", "main.cpp");
  cg->getMain()->setHasBody(true);

  const std::string generatorName = "Test";
  const metacg::MCGFileInfo mcgFileInfo = {{3, 0}, {generatorName, 0, 1, "TestSha"}};
  metacg::io::VersionThreeMCGWriter mcgWriter(mcgFileInfo);
  metacg::io::JsonSink jsonSink;
  mcgWriter.write(mcgm.getCallgraph("emptyGraph"), jsonSink);
  EXPECT_EQ(jsonSink.getJson().dump(),
            "{\"_CG\":{\"edges\":[],\"nodes\":[]},"
            "\"_MetaCG\":{\"generator\":{\"name\":\"Test\",\"sha\":\"TestSha\",\"version\":\"0.1\"},\"version\":\"3."
            "0\"}}");

  mcgWriter.write(mcgm.getCallgraph("newGraph"), jsonSink);
  EXPECT_EQ(jsonSink.getJson().dump(),
            "{\"_CG\":{\"edges\":[],\"nodes\":["
            "[13570296075836900426,{\"functionName\":\"main\",\"hasBody\":true,\"meta\":null,\"origin\":\"main.cpp\"}]"
            "]},\"_MetaCG\":{\"generator\":{\"name\":\"Test\",\"sha\":\"TestSha\",\"version\":\"0.1\"},\"version\":\"3."
            "0\"}}");
}

TEST_F(V3MCGWriterTest, WritePointerDebug) {
  auto& mcgm = metacg::graph::MCGManager::get();
  mcgm.addToManagedGraphs("newGraph", std::make_unique<metacg::Callgraph>());
  const auto& cg = mcgm.getCallgraph();
  cg->insert("main", "main.cpp");
  cg->getMain()->setHasBody(true);

  const std::string generatorName = "Test";
  const metacg::MCGFileInfo mcgFileInfo = {{3, 0}, {generatorName, 0, 1, "TestSha"}};
  metacg::io::VersionThreeMCGWriter mcgWriter(mcgFileInfo, true);
  metacg::io::JsonSink jsonSink;
  mcgWriter.write(mcgm.getCallgraph("emptyGraph"), jsonSink);
  EXPECT_EQ(jsonSink.getJson().dump(),
            "{\"_CG\":{\"edges\":[],\"nodes\":[]},"
            "\"_MetaCG\":{\"generator\":{\"name\":\"Test\",\"sha\":\"TestSha\",\"version\":\"0.1\"},\"version\":\"3."
            "0\"}}");

  mcgWriter.write(mcgm.getCallgraph("newGraph"), jsonSink);
  EXPECT_EQ(jsonSink.getJson().dump(),
            "{\"_CG\":{\"edges\":[],\"nodes\":["
            "[\"main\",{\"callees\":[],\"callers\":[],\"functionName\":\"main\",\"hasBody\":true,\"meta\":null,"
            "\"origin\":\"main.cpp\"}]"
            "]},\"_MetaCG\":{\"generator\":{\"name\":\"Test\",\"sha\":\"TestSha\",\"version\":\"0.1\"},\"version\":\"3."
            "0\"}}");
}

TEST_F(V3MCGWriterTest, SwitchBeforeWrite) {
  auto& mcgm = metacg::graph::MCGManager::get();
  mcgm.addToManagedGraphs("newGraph", std::make_unique<metacg::Callgraph>());
  const auto& cg = mcgm.getCallgraph();
  cg->insert("main", "main.cpp");
  cg->getMain()->setHasBody(true);

  const std::string generatorName = "Test";
  const metacg::MCGFileInfo mcgFileInfo = {{3, 0}, {generatorName, 0, 1, "TestSha"}};
  metacg::io::VersionThreeMCGWriter mcgWriter(mcgFileInfo);
  metacg::io::JsonSink jsonSink;

  mcgm.setActive("emptyGraph");
  mcgm.setActive("newGraph");
  mcgm.setActive("emptyGraph");

  mcgWriter.write(jsonSink);
  EXPECT_EQ(jsonSink.getJson().dump(),
            "{\"_CG\":{\"edges\":[],\"nodes\":[]},"
            "\"_MetaCG\":{\"generator\":{\"name\":\"Test\",\"sha\":\"TestSha\",\"version\":\"0.1\"},\"version\":\"3."
            "0\"}}");

  mcgm.setActive("newGraph");

  mcgWriter.write(jsonSink);
  EXPECT_EQ(jsonSink.getJson().dump(),
            "{\"_CG\":{\"edges\":[],\"nodes\":["
            "[13570296075836900426,{\"functionName\":\"main\",\"hasBody\":true,\"meta\":null,\"origin\":\"main.cpp\"}]"
            "]},\"_MetaCG\":{\"generator\":{\"name\":\"Test\",\"sha\":\"TestSha\",\"version\":\"0.1\"},\"version\":\"3."
            "0\"}}");
}
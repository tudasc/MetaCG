/**
 * File: VersionTwoReaderWriterRoundtripTest.cpp
 * License: Part of the metacg project. Licensed under BSD 3 clause license. See LICENSE.txt file at
 * https://github.com/tudasc/metacg/LICENSE.txt
 */

#include "LoggerUtil.h"
#include "MCGManager.h"
#include "TestMD.h"
#include "io/MCGWriter.h"
#include "io/VersionFourMCGReader.h"
#include "io/VersionFourMCGWriter.h"
#include "gtest/gtest.h"

class V4ReaderWriterRoundtripTest : public ::testing::Test {
 protected:
  void SetUp() override {
    auto& mcgm = metacg::graph::MCGManager::get();
    mcgm.resetManager();
  }
};

TEST_F(V4ReaderWriterRoundtripTest, TextGraphText) {
  const nlohmann::json jsonCG =
      "{\"_CG\":{\"meta\":{},\"nodes\":{\"0\":{\"callees\":{\"1\":{}},\"functionName\":\"main\",\"hasBody\":true,"
      "\"meta\":{},"
      "\"origin\":\"main.cpp\"},\"1\":{\"callees\":{},\"functionName\":\"foo\",\"hasBody\":true,\"meta\":{},"
      "\"origin\":\"main.cpp\"},\"2\":{\"callees\":{},\"functionName\":\"bar\",\"hasBody\":false,\"meta\":{},"
      "\"origin\":\"bar.cpp\"}}},\"_MetaCG\":{\"generator\":{\"name\":\"Test\",\"sha\":\"TestSha\",\"version\":\"0.1\"}"
      ","
      "\"version\":\"4.0\"}}"_json;

  metacg::io::JsonSource jsonSource(jsonCG);
  metacg::io::VersionFourMCGReader mcgReader(jsonSource);
  auto& mcgm = metacg::graph::MCGManager::get();
  mcgm.addToManagedGraphs("newGraph", mcgReader.read());

  const std::string generatorName = "Test";
  const metacg::MCGFileInfo mcgFileInfo = {{4, 0}, {generatorName, 0, 1, "TestSha"}};
  metacg::io::VersionFourMCGWriter mcgWriter(mcgFileInfo, false, true);
  metacg::io::JsonSink jsonSink;
  mcgWriter.writeActiveGraph(jsonSink);

  EXPECT_EQ(jsonSink.getJson(), jsonCG);
}

TEST_F(V4ReaderWriterRoundtripTest, TextGraphTextUseName) {
  const nlohmann::json jsonCG =
      "{\"_CG\":{\"meta\":{},\"nodes\":{\"main\":{\"callees\":{\"foo\":{}},\"functionName\":\"main\",\"hasBody\":true,"
      "\"meta\":{},"
      "\"origin\":\"main.cpp\"},\"foo\":{\"callees\":{},\"functionName\":\"foo\",\"hasBody\":true,\"meta\":{},"
      "\"origin\":\"main.cpp\"},\"bar\":{\"callees\":{},\"functionName\":\"bar\",\"hasBody\":false,\"meta\":{},"
      "\"origin\":\"bar.cpp\"}}},\"_MetaCG\":{\"generator\":{\"name\":\"Test\",\"sha\":\"TestSha\",\"version\":\"0.1\"}"
      ","
      "\"version\":\"4.0\"}}"_json;

  metacg::io::JsonSource jsonSource(jsonCG);
  metacg::io::VersionFourMCGReader mcgReader(jsonSource);
  auto& mcgm = metacg::graph::MCGManager::get();
  mcgm.addToManagedGraphs("newGraph", mcgReader.read());

  const std::string generatorName = "Test";
  const metacg::MCGFileInfo mcgFileInfo = {{4, 0}, {generatorName, 0, 1, "TestSha"}};
  metacg::io::VersionFourMCGWriter mcgWriter(mcgFileInfo, true, true);
  metacg::io::JsonSink jsonSink;
  mcgWriter.writeActiveGraph(jsonSink);

  EXPECT_EQ(jsonSink.getJson(), jsonCG);
}

TEST_F(V4ReaderWriterRoundtripTest, TextGraphTextWithMetadata) {
  const nlohmann::json jsonCG =
      "{\"_CG\":{\"meta\":{},\"nodes\":{\"0\":{\"callees\":{\"1\":{}},\"functionName\":\"main\",\"hasBody\":true,"
      "\"meta\":"
      "{\"SimpleTestMD\":{\"stored_double\":1337.0,\"stored_int\":0,\"stored_string\":\"TestString\"}},"
      "\"origin\":\"main.cpp\"},\"1\":{\"callees\":{},\"functionName\":\"foo\",\"hasBody\":true,\"meta\":{},"
      "\"origin\":\"main.cpp\"},\"2\":{\"callees\":{},\"functionName\":\"bar\",\"hasBody\":false,\"meta\":{},"
      "\"origin\":\"bar.cpp\"}}},\"_MetaCG\":{\"generator\":{\"name\":\"Test\",\"sha\":\"TestSha\",\"version\":\"0.1\"}"
      ","
      "\"version\":\"4.0\"}}"_json;

  metacg::io::JsonSource jsonSource(jsonCG);
  metacg::io::VersionFourMCGReader mcgReader(jsonSource);
  auto& mcgm = metacg::graph::MCGManager::get();
  mcgm.addToManagedGraphs("newGraph", mcgReader.read());

  const std::string generatorName = "Test";
  const metacg::MCGFileInfo mcgFileInfo = {{4, 0}, {generatorName, 0, 1, "TestSha"}};
  metacg::io::VersionFourMCGWriter mcgWriter(mcgFileInfo, false, true);
  metacg::io::JsonSink jsonSink;
  mcgWriter.writeActiveGraph(jsonSink);

  EXPECT_EQ(jsonSink.getJson(), jsonCG);
}

TEST_F(V4ReaderWriterRoundtripTest, TextGraphTextWithRefMetadata) {
  const nlohmann::json jsonCG =
      "{\"_CG\":{\"meta\":{},\"nodes\":{\"0\":{\"callees\":{\"1\":{}},\"functionName\":\"main\",\"hasBody\":true,"
      "\"meta\":"
      "{\"RefTestMD\":{\"node_ref\":\"1\"}},"
      "\"origin\":\"main.cpp\"},\"1\":{\"callees\":{},\"functionName\":\"foo\",\"hasBody\":true,\"meta\":"
      "{\"RefTestMD\":{\"node_ref\":\"2\"}},"
      "\"origin\":\"main.cpp\"},\"2\":{\"callees\":{},\"functionName\":\"bar\",\"hasBody\":false,\"meta\":"
      "{\"RefTestMD\":{\"node_ref\":\"0\"}},"
      "\"origin\":\"bar.cpp\"}}},\"_MetaCG\":{\"generator\":{\"name\":\"Test\",\"sha\":\"TestSha\",\"version\":\"0.1\"}"
      ","
      "\"version\":\"4.0\"}}"_json;

  metacg::io::JsonSource jsonSource(jsonCG);
  metacg::io::VersionFourMCGReader mcgReader(jsonSource);
  auto& mcgm = metacg::graph::MCGManager::get();
  mcgm.addToManagedGraphs("newGraph", mcgReader.read());

  const std::string generatorName = "Test";
  const metacg::MCGFileInfo mcgFileInfo = {{4, 0}, {generatorName, 0, 1, "TestSha"}};
  metacg::io::VersionFourMCGWriter mcgWriter(mcgFileInfo, false, true);
  metacg::io::JsonSink jsonSink;
  mcgWriter.writeActiveGraph(jsonSink);

  EXPECT_EQ(jsonSink.getJson(), jsonCG);
}
/**
 * File: VersionTwoReaderWriterRoundtripTest.cpp
 * License: Part of the metacg project. Licensed under BSD 3 clause license. See LICENSE.txt file at
 * https://github.com/tudasc/metacg/LICENSE.txt
 */

#include "LoggerUtil.h"
#include "MCGManager.h"
#include "io/MCGWriter.h"
#include "io/VersionThreeMCGReader.h"
#include "io/VersionThreeMCGWriter.h"
#include "gtest/gtest.h"

class V3ReaderWriterRoundtripTest : public ::testing::Test {
 protected:
  void SetUp() override {
    metacg::loggerutil::getLogger();
    auto& mcgm = metacg::graph::MCGManager::get();
    mcgm.resetManager();
  }
};

TEST_F(V3ReaderWriterRoundtripTest, TextGraphText) {
  const nlohmann::json jsonCG =
      "{\"_CG\":{"
      "\"edges\":["
      "[[11868120863286193964,9631199822919835226],null]"
      "],"
      "\"nodes\":["
      "[9631199822919835226,{\"functionName\":\"foo\",\"hasBody\":true,\"meta\":null,\"origin\":\"main.cpp\"}],"
      "[11474628671133349555,{\"functionName\":\"bar\",\"hasBody\":false,\"meta\":null,\"origin\":\"bar.cpp\"}],"
      "[11868120863286193964,{\"functionName\":\"main\",\"hasBody\":true,\"meta\":null,\"origin\":\"main.cpp\"}]"
      "]},"
      "\"_MetaCG\":{\"generator\":{\"name\":\"Test\",\"sha\":\"TestSha\",\"version\":\"0.1\"},\"version\":\"3.0\"}}"_json;

  metacg::io::JsonSource jsonSource(jsonCG);
  metacg::io::VersionThreeMetaCGReader mcgReader(jsonSource);
  auto& mcgm = metacg::graph::MCGManager::get();
  mcgm.addToManagedGraphs("newGraph", mcgReader.read());

  const std::string generatorName = "Test";
  const metacg::MCGFileInfo mcgFileInfo = {{3, 0}, {generatorName, 0, 1, "TestSha"}};
  metacg::io::VersionThreeMCGWriter mcgWriter(mcgFileInfo, false, true);
  metacg::io::JsonSink jsonSink;
  mcgWriter.writeActiveGraph(jsonSink);

  EXPECT_EQ(jsonSink.getJson(), jsonCG);
}

TEST_F(V3ReaderWriterRoundtripTest, DebugTextGraphDebugText) {
  const nlohmann::json jsonCG =
      "{\"_CG\":{"
      "\"edges\":[[[13570296075836900426,6731461454900136138],null]],"
      "\"nodes\":["
      "[\"foo\",{\"callees\":[],\"callers\":[\"main\"],\"functionName\":\"foo\",\"hasBody\":true,\"meta\":null,"
      "\"origin\":\"main.cpp\"}],"
      "[\"main\",{\"callees\":[\"foo\"],\"callers\":[],\"functionName\":\"main\",\"hasBody\":true,\"meta\":null,"
      "\"origin\":\"main.cpp\"}],"
      "[\"bar\",{\"callees\":[],\"callers\":[],\"functionName\":\"bar\",\"hasBody\":false,\"meta\":null,\"origin\":"
      "\"bar.cpp\"}]]},"
      "\"_MetaCG\":{\"generator\":{\"name\":\"Test\",\"sha\":\"TestSha\",\"version\":\"0.1\"},\"version\":\"3.0\"}}"_json;

  metacg::io::JsonSource jsonSource(jsonCG);
  metacg::io::VersionThreeMetaCGReader mcgReader(jsonSource);
  auto& mcgm = metacg::graph::MCGManager::get();
  mcgm.addToManagedGraphs("newGraph", mcgReader.read());

  const std::string generatorName = "Test";
  const metacg::MCGFileInfo mcgFileInfo = {{3, 0}, {generatorName, 0, 1, "TestSha"}};
  metacg::io::VersionThreeMCGWriter mcgWriter(mcgFileInfo, true, true);
  metacg::io::JsonSink jsonSink;
  mcgWriter.writeActiveGraph(jsonSink);

  EXPECT_EQ(jsonSink.getJson(), jsonCG);
}

TEST_F(V3ReaderWriterRoundtripTest, DebugTextGraphText) {
  const nlohmann::json jsonDbgCG =
      "{\"_CG\":{"
      "\"edges\":[[[13570296075836900426,6731461454900136138],null]],"
      "\"nodes\":["
      "[\"foo\",{\"callees\":[],\"callers\":[\"main\"],\"functionName\":\"foo\",\"hasBody\":true,\"meta\":null,"
      "\"origin\":\"main.cpp\"}],"
      "[\"bar\",{\"callees\":[],\"callers\":[],\"functionName\":\"bar\",\"hasBody\":false,\"meta\":null,\"origin\":"
      "\"bar.cpp\"}],"
      "[\"main\",{\"callees\":[\"foo\"],\"callers\":[],\"functionName\":\"main\",\"hasBody\":true,\"meta\":null,"
      "\"origin\":\"main.cpp\"}]]},"
      "\"_MetaCG\":{\"generator\":{\"name\":\"Test\",\"sha\":\"TestSha\",\"version\":\"0.1\"},\"version\":\"3.0\"}}"_json;

  metacg::io::JsonSource jsonSource(jsonDbgCG);
  metacg::io::VersionThreeMetaCGReader mcgReader(jsonSource);
  auto& mcgm = metacg::graph::MCGManager::get();
  mcgm.addToManagedGraphs("newGraph", mcgReader.read());

  const std::string generatorName = "Test";
  const metacg::MCGFileInfo mcgFileInfo = {{3, 0}, {generatorName, 0, 1, "TestSha"}};
  metacg::io::VersionThreeMCGWriter mcgWriter(mcgFileInfo, false, true);
  metacg::io::JsonSink jsonSink;
  mcgWriter.writeActiveGraph(jsonSink);

  const nlohmann::json jsonCG =
      "{\"_CG\":{"
      "\"edges\":["
      "[[13570296075836900426,6731461454900136138],null]"
      "],"
      "\"nodes\":["
      "[6731461454900136138,{\"functionName\":\"foo\",\"hasBody\":true,\"meta\":null,\"origin\":\"main.cpp\"}],"
      "[13570296075836900426,{\"functionName\":\"main\",\"hasBody\":true,\"meta\":null,\"origin\":\"main.cpp\"}],"
      "[17384295136535843005,{\"functionName\":\"bar\",\"hasBody\":false,\"meta\":null,\"origin\":\"bar.cpp\"}]"
      "]},"
      "\"_MetaCG\":{\"generator\":{\"name\":\"Test\",\"sha\":\"TestSha\",\"version\":\"0.1\"},\"version\":\"3.0\"}}"_json;

  EXPECT_EQ(jsonSink.getJson(), jsonCG);
}

TEST_F(V3ReaderWriterRoundtripTest, TextGraphDebugText) {
  const nlohmann::json jsonCG =
      "{\"_CG\":{"
      "\"edges\":["
      "[[11868120863286193964,9631199822919835226],null]"
      "],"
      "\"nodes\":["
      "[9631199822919835226,{\"functionName\":\"foo\",\"hasBody\":true,\"meta\":null,\"origin\":\"main.cpp\"}],"
      "[11474628671133349555,{\"functionName\":\"bar\",\"hasBody\":false,\"meta\":null,\"origin\":\"bar.cpp\"}],"
      "[11868120863286193964,{\"functionName\":\"main\",\"hasBody\":true,\"meta\":null,\"origin\":\"main.cpp\"}]"
      "]},"
      "\"_MetaCG\":{\"generator\":{\"name\":\"Test\",\"sha\":\"TestSha\",\"version\":\"0.1\"},\"version\":\"3.0\"}}"_json;

  metacg::io::JsonSource jsonSource(jsonCG);
  metacg::io::VersionThreeMetaCGReader mcgReader(jsonSource);
  auto& mcgm = metacg::graph::MCGManager::get();
  mcgm.addToManagedGraphs("newGraph", mcgReader.read());

  const std::string generatorName = "Test";
  const metacg::MCGFileInfo mcgFileInfo = {{3, 0}, {generatorName, 0, 1, "TestSha"}};
  metacg::io::VersionThreeMCGWriter mcgWriter(mcgFileInfo, true, true);
  metacg::io::JsonSink jsonSink;
  mcgWriter.writeActiveGraph(jsonSink);

  const nlohmann::json jsonDbgCG =
      "{\"_CG\":{"
      "\"edges\":[[[11868120863286193964,9631199822919835226],null]],"
      "\"nodes\":["
      "[\"foo\",{\"callees\":[],\"callers\":[\"main\"],\"functionName\":\"foo\",\"hasBody\":true,\"meta\":null,"
      "\"origin\":\"main.cpp\"}],"
      "[\"bar\",{\"callees\":[],\"callers\":[],\"functionName\":\"bar\",\"hasBody\":false,\"meta\":null,\"origin\":"
      "\"bar.cpp\"}],"
      "[\"main\",{\"callees\":[\"foo\"],\"callers\":[],\"functionName\":\"main\",\"hasBody\":true,\"meta\":null,"
      "\"origin\":\"main.cpp\"}]]},"
      "\"_MetaCG\":{\"generator\":{\"name\":\"Test\",\"sha\":\"TestSha\",\"version\":\"0.1\"},\"version\":\"3.0\"}}"_json;

  EXPECT_EQ(jsonSink.getJson(), jsonDbgCG);
}
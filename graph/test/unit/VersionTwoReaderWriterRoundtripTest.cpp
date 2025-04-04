/**
 * File: VersionTwoReaderWriterRoundtripTest.cpp
 * License: Part of the metacg project. Licensed under BSD 3 clause license. See LICENSE.txt file at
 * https://github.com/tudasc/metacg/LICENSE.txt
 */

#include "LoggerUtil.h"
#include "MCGManager.h"
#include "io/MCGWriter.h"
#include "io/VersionTwoMCGReader.h"
#include "io/VersionTwoMCGWriter.h"
#include "gtest/gtest.h"

class VersionTwoReaderWriterRoundtripTest : public ::testing::Test {
 protected:
  void SetUp() override {
    auto& mcgm = metacg::graph::MCGManager::get();
    mcgm.resetManager();
  }
};

TEST_F(VersionTwoReaderWriterRoundtripTest, TextGraphText) {
  const nlohmann::json jsonCG =
      "{\n"
      "   \"_CG\":{\n"
      "      \"main\":{\n"
      "         \"callees\":[\n"
      "            \"foo\"\n"
      "         ],\n"
      "         \"callers\":[],\n"
      "         \"doesOverride\":false,\n"
      "         \"hasBody\":true,\n"
      "         \"isVirtual\":false,\n"
      "         \"meta\":{\"fileProperties\":{\"origin\":\"main.cpp\",\"systemInclude\":false}},\n"
      "         \"overriddenBy\":[],\n"
      "         \"overrides\":[]\n"
      "      },\n"
      "      \"foo\":{\n"
      "         \"callees\":[],\n"
      "         \"callers\":[\n"
      "            \"main\"\n"
      "         ],\n"
      "         \"doesOverride\":false,\n"
      "         \"hasBody\":false,\n"
      "         \"isVirtual\":true,\n"
      "         \"meta\":null,\n"
      "         \"overriddenBy\":[\"bar\"],\n"
      "         \"overrides\":[]\n"
      "      },\n"
      "      \"bar\":{\n"
      "         \"callees\":[],\n"
      "         \"callers\":[],\n"
      "         \"doesOverride\":true,\n"
      "         \"hasBody\":true,\n"
      "         \"isVirtual\":true,\n"
      "         \"meta\":{\"fileProperties\":{\"origin\":\"bar.cpp\",\"systemInclude\":false}},\n"
      "         \"overriddenBy\":[],\n"
      "         \"overrides\":[\"foo\"]\n"
      "      }\n"
      "   },\n"
      "   \"_MetaCG\":{\n"
      "      \"generator\":{\n"
      "         \"name\":\"Test\",\n"
      "         \"sha\":\"TestSha\",\n"
      "         \"version\":\"0.1\"\n"
      "      },\n"
      "      \"version\":\"2.0\"\n"
      "   }\n"
      "}"_json;

  metacg::io::JsonSource jsonSource(jsonCG);
  metacg::io::VersionTwoMetaCGReader reader(jsonSource);
  auto& mcgm = metacg::graph::MCGManager::get();
  mcgm.addToManagedGraphs("newCallgraph", reader.read());
  const std::string generatorName = "Test";
  const metacg::MCGFileInfo mcgFileInfo = {{2, 0}, {generatorName, 0, 1, "TestSha"}};
  metacg::io::VersionTwoMCGWriter mcgWriter(mcgFileInfo);
  metacg::io::JsonSink jsonSink;
  mcgWriter.writeActiveGraph(jsonSink);

  EXPECT_EQ(jsonSink.getJson(), jsonCG);
}

// If the source file does include a fileProperty metadata field which does not carry information,
// we do not generate a fileProperty metadata filed with now information when outputting the file again.
// This is the current behaviour and subject to change. This test is here to find unwanted changes to this behaviour.
TEST_F(VersionTwoReaderWriterRoundtripTest, EmptyFilePropertyMetadata) {
  const nlohmann::json jsonCGin =
      "{\n"
      "   \"_CG\":{\n"
      "      \"main\":{\n"
      "         \"callees\":[\n"
      "            \"foo\"\n"
      "         ],\n"
      "         \"callers\":[],\n"
      "         \"doesOverride\":false,\n"
      "         \"hasBody\":true,\n"
      "         \"isVirtual\":false,\n"
      "         \"meta\":{\"fileProperties\":{\"origin\":\"\",\"systemInclude\":false}},\n"
      "         \"overriddenBy\":[],\n"
      "         \"overrides\":[]\n"
      "      },\n"
      "      \"foo\":{\n"
      "         \"callees\":[],\n"
      "         \"callers\":[\n"
      "            \"main\"\n"
      "         ],\n"
      "         \"doesOverride\":false,\n"
      "         \"hasBody\":false,\n"
      "         \"isVirtual\":true,\n"
      "         \"meta\":null,\n"
      "         \"overriddenBy\":[\"bar\"],\n"
      "         \"overrides\":[]\n"
      "      },\n"
      "      \"bar\":{\n"
      "         \"callees\":[],\n"
      "         \"callers\":[],\n"
      "         \"doesOverride\":true,\n"
      "         \"hasBody\":true,\n"
      "         \"isVirtual\":true,\n"
      "         \"meta\":{\"fileProperties\":{\"origin\":\"\",\"systemInclude\":false}},\n"
      "         \"overriddenBy\":[],\n"
      "         \"overrides\":[\"foo\"]\n"
      "      }\n"
      "   },\n"
      "   \"_MetaCG\":{\n"
      "      \"generator\":{\n"
      "         \"name\":\"Test\",\n"
      "         \"sha\":\"TestSha\",\n"
      "         \"version\":\"0.1\"\n"
      "      },\n"
      "      \"version\":\"2.0\"\n"
      "   }\n"
      "}"_json;

  const nlohmann::json jsonCGOut =
      "{\n"
      "   \"_CG\":{\n"
      "      \"main\":{\n"
      "         \"callees\":[\n"
      "            \"foo\"\n"
      "         ],\n"
      "         \"callers\":[],\n"
      "         \"doesOverride\":false,\n"
      "         \"hasBody\":true,\n"
      "         \"isVirtual\":false,\n"
      "         \"meta\": null,\n"
      "         \"overriddenBy\":[],\n"
      "         \"overrides\":[]\n"
      "      },\n"
      "      \"foo\":{\n"
      "         \"callees\":[],\n"
      "         \"callers\":[\n"
      "            \"main\"\n"
      "         ],\n"
      "         \"doesOverride\":false,\n"
      "         \"hasBody\":false,\n"
      "         \"isVirtual\":true,\n"
      "         \"meta\":null,\n"
      "         \"overriddenBy\":[\"bar\"],\n"
      "         \"overrides\":[]\n"
      "      },\n"
      "      \"bar\":{\n"
      "         \"callees\":[],\n"
      "         \"callers\":[],\n"
      "         \"doesOverride\":true,\n"
      "         \"hasBody\":true,\n"
      "         \"isVirtual\":true,\n"
      "         \"meta\": null,\n"
      "         \"overriddenBy\":[],\n"
      "         \"overrides\":[\"foo\"]\n"
      "      }\n"
      "   },\n"
      "   \"_MetaCG\":{\n"
      "      \"generator\":{\n"
      "         \"name\":\"Test\",\n"
      "         \"sha\":\"TestSha\",\n"
      "         \"version\":\"0.1\"\n"
      "      },\n"
      "      \"version\":\"2.0\"\n"
      "   }\n"
      "}"_json;

  metacg::io::JsonSource jsonSource(jsonCGin);
  metacg::io::VersionTwoMetaCGReader reader(jsonSource);
  auto& mcgm = metacg::graph::MCGManager::get();
  mcgm.addToManagedGraphs("newCallgraph", reader.read());
  const std::string generatorName = "Test";
  const metacg::MCGFileInfo mcgFileInfo = {{2, 0}, {generatorName, 0, 1, "TestSha"}};
  metacg::io::VersionTwoMCGWriter mcgWriter(mcgFileInfo);
  metacg::io::JsonSink jsonSink;
  mcgWriter.writeActiveGraph(jsonSink);

  EXPECT_EQ(jsonSink.getJson(), jsonCGOut);
}
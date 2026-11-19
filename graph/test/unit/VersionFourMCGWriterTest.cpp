/**
 * File: VersionFourMCGWriterTest.cpp
 * License: Part of the metacg project. Licensed under BSD 3 clause license. See LICENSE.txt file at
 * https://github.com/tudasc/metacg/LICENSE.txt
 */

#include "metacg/LoggerUtil.h"
#include "gtest/gtest.h"

#include "TestMD.h"
#include "metacg/MCGBaseInfo.h"
#include "metacg/MCGManager.h"
#include "metacg/io/VersionFourMCGWriter.h"
#include "metacg/metadata/EntryFunctionMD.h"

class V4MCGWriterTest : public ::testing::Test {
 protected:
  void SetUp() override {
    auto& mcgm = metacg::graph::MCGManager::get();
    mcgm.resetManager();
    mcgm.addToManagedGraphs("emptyGraph", std::make_unique<metacg::Callgraph>());
  }
};

TEST_F(V4MCGWriterTest, DifferentMetaInformation) {
  const std::string generatorName = "Test";
  const metacg::MCGFileInfo mcgFileInfo = {{4, 0}, {generatorName, 0, 1, "TestSha"}};
  metacg::io::VersionFourMCGWriter mcgWriter(mcgFileInfo);
  metacg::io::JsonSink jsonSink;
  mcgWriter.writeActiveGraph(jsonSink);
  EXPECT_EQ(jsonSink.getJson().dump(),
            "{\"_CG\":{\"meta\":{},\"nodes\":{}},\"_MetaCG\":{\"generator\":{\"name\":\"Test\",\"sha\":\"TestSha\","
            "\"version\":\"0.1\"},"
            "\"version\":\"4.0\"}}");
}

TEST_F(V4MCGWriterTest, OneNodeCGWrite) {
  auto& mcgm = metacg::graph::MCGManager::get();
  const auto& cg = mcgm.getCallgraph();
  cg->insert("main", "main.cpp");
  cg->getMain()->setHasBody(true);

  const std::string generatorName = "Test";
  const metacg::MCGFileInfo mcgFileInfo = {{4, 0}, {generatorName, 0, 1, "TestSha"}};
  metacg::io::VersionFourMCGWriter mcgWriter(mcgFileInfo);
  metacg::io::JsonSink jsonSink;
  mcgWriter.writeActiveGraph(jsonSink);

  EXPECT_EQ(jsonSink.getJson().dump(),
            "{\"_CG\":{\"meta\":{},\"nodes\":{\"0\":{\"callees\":{},\"functionName\":\"main\",\"hasBody\":true,"
            "\"meta\":{},\"origin\":"
            "\"main.cpp\"}}},\"_MetaCG\":{\"generator\":{\"name\":\"Test\",\"sha\":\"TestSha\",\"version\":\"0.1\"},"
            "\"version\":\"4.0\"}}");
}

TEST_F(V4MCGWriterTest, OneNodeCGWriteUseName) {
  auto& mcgm = metacg::graph::MCGManager::get();
  const auto& cg = mcgm.getCallgraph();
  cg->insert("main", "main.cpp");
  cg->getMain()->setHasBody(true);

  const std::string generatorName = "Test";
  const metacg::MCGFileInfo mcgFileInfo = {{4, 0}, {generatorName, 0, 1, "TestSha"}};
  metacg::io::VersionFourMCGWriter mcgWriter(mcgFileInfo, true);

  metacg::io::JsonSink jsonSink;
  mcgWriter.writeActiveGraph(jsonSink);

  EXPECT_EQ(jsonSink.getJson().dump(),
            "{\"_CG\":{\"meta\":{},\"nodes\":{\"main\":{\"callees\":{},\"functionName\":\"main\",\"hasBody\":true,"
            "\"meta\":{},"
            "\"origin\":\"main.cpp\"}}},\"_MetaCG\":{\"generator\":{\"name\":\"Test\",\"sha\":\"TestSha\",\"version\":"
            "\"0.1\"},\"version\":\"4.0\"}}");
}

TEST_F(V4MCGWriterTest, TwoNodeCGWrite) {
  auto& mcgm = metacg::graph::MCGManager::get();
  const auto& cg = mcgm.getCallgraph();
  cg->insert("main", "main.cpp");
  cg->getMain()->setHasBody(true);

  cg->insert("foo", "main.cpp");
  cg->getSingleNode("foo").setHasBody(true);

  const std::string generatorName = "Test";
  const metacg::MCGFileInfo mcgFileInfo = {{4, 0}, {generatorName, 0, 1, "TestSha"}};
  metacg::io::VersionFourMCGWriter mcgWriter(mcgFileInfo);
  metacg::io::JsonSink jsonSink;
  mcgWriter.writeActiveGraph(jsonSink);
  EXPECT_EQ(
      jsonSink.getJson().dump(),
      "{\"_CG\":{\"meta\":{},\"nodes\":{\"0\":{\"callees\":{},\"functionName\":\"main\",\"hasBody\":true,\"meta\":{},"
      "\"origin\":\"main."
      "cpp\"},\"1\":{\"callees\":{},\"functionName\":\"foo\",\"hasBody\":true,\"meta\":{},\"origin\":\"main."
      "cpp\"}}}"
      ",\"_MetaCG\":{\"generator\":{\"name\":\"Test\",\"sha\":\"TestSha\",\"version\":\"0.1\"},\"version\":\"4.0\"}}");
}

TEST_F(V4MCGWriterTest, TwoNodeCGWriteUseName) {
  auto& mcgm = metacg::graph::MCGManager::get();
  const auto& cg = mcgm.getCallgraph();
  cg->insert("main", "main.cpp");
  cg->getMain()->setHasBody(true);

  cg->insert("foo", "main.cpp");
  cg->getSingleNode("foo").setHasBody(true);

  const std::string generatorName = "Test";
  const metacg::MCGFileInfo mcgFileInfo = {{4, 0}, {generatorName, 0, 1, "TestSha"}};
  metacg::io::VersionFourMCGWriter mcgWriter(mcgFileInfo, true);
  metacg::io::JsonSink jsonSink;
  mcgWriter.writeActiveGraph(jsonSink);
  EXPECT_EQ(jsonSink.getJson().dump(),
            "{\"_CG\":{\"meta\":{},\"nodes\":{\"foo\":{\"callees\":{},\"functionName\":\"foo\",\"hasBody\":true,"
            "\"meta\":{},\"origin\":"
            "\"main.cpp\"},\"main\":{\"callees\":{},\"functionName\":\"main\",\"hasBody\":true,\"meta\":{},"
            "\"origin\":\"main.cpp\"}}},\"_MetaCG\":{\"generator\":{\"name\":\"Test\",\"sha\":\"TestSha\",\"version\":"
            "\"0.1\"},\"version\":\"4.0\"}}");
}

TEST_F(V4MCGWriterTest, TwoNodeOneEdgeCGWrite) {
  auto& mcgm = metacg::graph::MCGManager::get();
  const auto& cg = mcgm.getCallgraph();
  cg->insert("main", "main.cpp");
  cg->getMain()->setHasBody(true);

  cg->insert("foo", "main.cpp");
  cg->getSingleNode("foo").setHasBody(true);

  cg->addEdge("main", "foo");

  const std::string generatorName = "Test";
  const metacg::MCGFileInfo mcgFileInfo = {{4, 0}, {generatorName, 0, 1, "TestSha"}};
  metacg::io::VersionFourMCGWriter mcgWriter(mcgFileInfo);
  metacg::io::JsonSink jsonSink;
  mcgWriter.writeActiveGraph(jsonSink);

  EXPECT_EQ(jsonSink.getJson().dump(),
            "{\"_CG\":{\"meta\":{},\"nodes\":{\"0\":{\"callees\":{\"1\":{}},\"functionName\":\"main\",\"hasBody\":true,"
            "\"meta\":{},"
            "\"origin\":\"main.cpp\"},\"1\":{\"callees\":{},\"functionName\":\"foo\",\"hasBody\":true,\"meta\":{},"
            "\"origin\":\"main.cpp\"}}},\"_MetaCG\":{\"generator\":{\"name\":\"Test\",\"sha\":\"TestSha\",\"version\":"
            "\"0.1\"},\"version\":\"4.0\"}}");
}

TEST_F(V4MCGWriterTest, TwoNodeOneEdgeCGWriteUseName) {
  auto& mcgm = metacg::graph::MCGManager::get();
  const auto& cg = mcgm.getCallgraph();
  cg->insert("main", "main.cpp");
  cg->getMain()->setHasBody(true);

  cg->insert("foo", "main.cpp");
  cg->getSingleNode("foo").setHasBody(true);

  cg->addEdge("main", "foo");

  const std::string generatorName = "Test";
  const metacg::MCGFileInfo mcgFileInfo = {{4, 0}, {generatorName, 0, 1, "TestSha"}};
  metacg::io::VersionFourMCGWriter mcgWriter(mcgFileInfo, true);
  metacg::io::JsonSink jsonSink;
  mcgWriter.writeActiveGraph(jsonSink);

  EXPECT_EQ(jsonSink.getJson().dump(),
            "{\"_CG\":{\"meta\":{},\"nodes\":{\"foo\":{\"callees\":{},\"functionName\":\"foo\",\"hasBody\":true,"
            "\"meta\":{},\"origin\":\"main."
            "cpp\"},\"main\":{\"callees\":{\"foo\":{}},\"functionName\":\"main\",\"hasBody\":true,\"meta\":{},"
            "\"origin\":\"main.cpp\"}}},\"_MetaCG\":{\"generator\":{\"name\":\"Test\",\"sha\":\"TestSha\",\"version\":"
            "\"0.1\"},\"version\":\"4.0\"}}");
}

TEST_F(V4MCGWriterTest, FourNodeOneEdgeCGWrite) {
  auto& mcgm = metacg::graph::MCGManager::get();
  const auto& cg = mcgm.getCallgraph();
  cg->insert("main", "main.cpp");
  cg->getMain()->setHasBody(true);

  cg->insert("foo", "main.cpp");
  cg->getSingleNode("foo").setHasBody(true);

  cg->insert("bar", "bar.cpp");
  cg->getSingleNode("bar").setHasBody(false);

  cg->addEdge("main", "foo");

  const std::string generatorName = "Test";
  const metacg::MCGFileInfo mcgFileInfo = {{4, 0}, {generatorName, 0, 1, "TestSha"}};
  metacg::io::VersionFourMCGWriter mcgWriter(mcgFileInfo);
  metacg::io::JsonSink jsonSink;
  mcgWriter.writeActiveGraph(jsonSink);

  EXPECT_EQ(
      jsonSink.getJson().dump(),
      "{\"_CG\":{\"meta\":{},\"nodes\":{\"0\":{\"callees\":{\"1\":{}},\"functionName\":\"main\",\"hasBody\":true,"
      "\"meta\":{},\"origin\":"
      "\"main.cpp\"},\"1\":{\"callees\":{},\"functionName\":\"foo\",\"hasBody\":true,\"meta\":{},\"origin\":\"main."
      "cpp\"},\"2\":{\"callees\":{},\"functionName\":\"bar\",\"hasBody\":false,\"meta\":{},\"origin\":\"bar.cpp\"}}},"
      "\"_"
      "MetaCG\":{\"generator\":{\"name\":\"Test\",\"sha\":\"TestSha\",\"version\":\"0.1\"},\"version\":\"4.0\"}}");
}

TEST_F(V4MCGWriterTest, FourNodeOneEdgeCGWriteUseName) {
  auto& mcgm = metacg::graph::MCGManager::get();
  const auto& cg = mcgm.getCallgraph();
  cg->insert("main", "main.cpp");
  cg->getMain()->setHasBody(true);

  cg->insert("foo", "main.cpp");
  cg->getSingleNode("foo").setHasBody(true);

  cg->insert("bar", "bar.cpp");
  cg->getSingleNode("bar").setHasBody(false);

  cg->addEdge("main", "foo");

  const std::string generatorName = "Test";
  const metacg::MCGFileInfo mcgFileInfo = {{4, 0}, {generatorName, 0, 1, "TestSha"}};
  metacg::io::VersionFourMCGWriter mcgWriter(mcgFileInfo, true);
  metacg::io::JsonSink jsonSink;
  mcgWriter.writeActiveGraph(jsonSink);

  EXPECT_EQ(jsonSink.getJson().dump(),
            "{\"_CG\":{\"meta\":{},\"nodes\":{\"bar\":{\"callees\":{},\"functionName\":\"bar\",\"hasBody\":false,"
            "\"meta\":{},\"origin\":\"bar."
            "cpp\"},\"foo\":{\"callees\":{},\"functionName\":\"foo\",\"hasBody\":true,\"meta\":{},\"origin\":\"main."
            "cpp\"},\"main\":{\"callees\":{\"foo\":{}},\"functionName\":\"main\",\"hasBody\":true,\"meta\":{},"
            "\"origin\":\"main.cpp\"}}},\"_MetaCG\":{\"generator\":{\"name\":\"Test\",\"sha\":\"TestSha\",\"version\":"
            "\"0.1\"},\"version\":\"4.0\"}}");
}

TEST_F(V4MCGWriterTest, GraphMetadataCGWrite) {
  auto& mcgm = metacg::graph::MCGManager::get();
  const auto& cg = mcgm.getCallgraph();
  cg->insert("main", "main.cpp");
  cg->getMain()->setHasBody(true);

  auto testMetaData = std::make_unique<SimpleTestMD>(0, 1337, "TestString");
  cg->getMain()->addMetaData(std::move(testMetaData));

  const std::string generatorName = "Test";
  const metacg::MCGFileInfo mcgFileInfo = {{4, 0}, {generatorName, 0, 1, "TestSha"}};
  metacg::io::VersionFourMCGWriter mcgWriter(mcgFileInfo);
  metacg::io::JsonSink jsonSink;
  mcgWriter.writeActiveGraph(jsonSink);

  EXPECT_EQ(
      jsonSink.getJson().dump(),
      "{\"_CG\":{\"meta\":{},\"nodes\":{\"0\":{\"callees\":{},\"functionName\":\"main\",\"hasBody\":true,\"meta\":{"
      "\"SimpleTestMD\":{"
      "\"stored_double\":1337.0,\"stored_int\":0,\"stored_string\":\"TestString\"}},\"origin\":\"main.cpp\"}}},\"_"
      "MetaCG\":{\"generator\":{\"name\":\"Test\",\"sha\":\"TestSha\",\"version\":\"0.1\"},\"version\":\"4.0\"}}");
}

TEST_F(V4MCGWriterTest, GraphMetadataWithRefWrite) {
  auto& mcgm = metacg::graph::MCGManager::get();
  const auto& cg = mcgm.getCallgraph();
  cg->insert("main", "main.cpp");
  cg->getMain()->setHasBody(true);

  auto& foo = cg->insert("foo", "main.cpp");
  foo.setHasBody(true);

  auto refMD = std::make_unique<RefTestMD>(foo);
  cg->getMain()->addMetaData(std::move(refMD));

  const std::string generatorName = "Test";
  const metacg::MCGFileInfo mcgFileInfo = {{4, 0}, {generatorName, 0, 1, "TestSha"}};
  metacg::io::VersionFourMCGWriter mcgWriter(mcgFileInfo);
  metacg::io::JsonSink jsonSink;
  mcgWriter.writeActiveGraph(jsonSink);

  EXPECT_EQ(
      jsonSink.getJson().dump(),
      "{\"_CG\":{\"meta\":{},\"nodes\":{\"0\":{\"callees\":{},\"functionName\":\"main\",\"hasBody\":true,\"meta\":{"
      "\"RefTestMD\":{"
      "\"node_ref\":\"1\"}},\"origin\":\"main.cpp\"},\"1\":{\"callees\":{},\"functionName\":\"foo\",\"hasBody\":true,"
      "\"meta\":{},\"origin\":\"main.cpp\"}}},\"_MetaCG\":{\"generator\":{\"name\":\"Test\",\"sha\":\"TestSha\","
      "\"version\":\"0.1\"},\"version\":\"4.0\"}}");
}

TEST_F(V4MCGWriterTest, GlobalGraphMetadataWrite) {
  auto& mcgm = metacg::graph::MCGManager::get();
  const auto& cg = mcgm.getCallgraph();
  auto& main = cg->insert("main", "main.cpp");
  main.setHasBody(true);

  auto entryFunctionMD = std::make_unique<metacg::EntryFunctionMD>(main);
  cg->addMetaData(std::move(entryFunctionMD));

  const std::string generatorName = "Test";
  const metacg::MCGFileInfo mcgFileInfo = {{4, 0}, {generatorName, 0, 1, "TestSha"}};
  metacg::io::VersionFourMCGWriter mcgWriter(mcgFileInfo);
  metacg::io::JsonSink jsonSink;
  mcgWriter.writeActiveGraph(jsonSink);

  EXPECT_EQ(
      jsonSink.getJson().dump(),
      "{\"_CG\":{\"meta\":{\"entryFunction\":\"0\"},\"nodes\":{\"0\":{\"callees\":{},\"functionName\":\"main\","
      "\"hasBody\":true,\"meta\":{},\"origin\":\"main.cpp\"}}},"
      "\"_MetaCG\":{\"generator\":{\"name\":\"Test\",\"sha\":\"TestSha\",\"version\":\"0.1\"},\"version\":\"4.0\"}}");
}

TEST_F(V4MCGWriterTest, EdgeMetadataCGWrite) {
  auto& mcgm = metacg::graph::MCGManager::get();
  const auto& cg = mcgm.getCallgraph();
  auto& main = cg->insert("main", "main.cpp");
  cg->getMain()->setHasBody(true);
  auto& foo = cg->insert("foo", "main.cpp");
  cg->getSingleNode("foo").setHasBody(true);

  cg->addEdge("main", "foo");

  auto testMetaData = std::make_unique<SimpleTestMD>(0, 1337, "TestString");
  cg->addEdgeMetaData(main, foo, std::move(testMetaData));

  const std::string generatorName = "Test";
  const metacg::MCGFileInfo mcgFileInfo = {{4, 0}, {generatorName, 0, 1, "TestSha"}};
  metacg::io::VersionFourMCGWriter mcgWriter(mcgFileInfo, false);
  metacg::io::JsonSink jsonSink;
  mcgWriter.writeActiveGraph(jsonSink);
  EXPECT_EQ(
      jsonSink.getJson().dump(),
      "{\"_CG\":{\"meta\":{},\"nodes\":{\"0\":{\"callees\":{\"1\":{\"SimpleTestMD\":{\"stored_double\":1337.0,\"stored_"
      "int\":0,\"stored_"
      "string\":\"TestString\"}}},\"functionName\":\"main\",\"hasBody\":true,\"meta\":{},\"origin\":\"main.cpp\"},"
      "\"1\":{\"callees\":{},\"functionName\":\"foo\",\"hasBody\":true,\"meta\":{},\"origin\":\"main.cpp\"}}},\"_"
      "MetaCG\":{\"generator\":{\"name\":\"Test\",\"sha\":\"TestSha\",\"version\":\"0.1\"},\"version\":\"4.0\"}}");
}

TEST_F(V4MCGWriterTest, WriteByName) {
  auto& mcgm = metacg::graph::MCGManager::get();
  mcgm.addToManagedGraphs("newGraph", std::make_unique<metacg::Callgraph>());
  const auto& cg = mcgm.getCallgraph();
  cg->insert("main", "main.cpp");
  cg->getMain()->setHasBody(true);

  const std::string generatorName = "Test";
  const metacg::MCGFileInfo mcgFileInfo = {{4, 0}, {generatorName, 0, 1, "TestSha"}};
  metacg::io::VersionFourMCGWriter mcgWriter(mcgFileInfo);
  metacg::io::JsonSink jsonSink;
  mcgWriter.writeNamedGraph("emptyGraph", jsonSink);
  EXPECT_EQ(jsonSink.getJson().dump(),
            "{\"_CG\":{\"meta\":{},\"nodes\":{}},"
            "\"_MetaCG\":{\"generator\":{\"name\":\"Test\",\"sha\":\"TestSha\",\"version\":\"0.1\"},\"version\":\"4."
            "0\"}}");

  mcgWriter.writeNamedGraph("newGraph", jsonSink);
  EXPECT_EQ(
      jsonSink.getJson().dump(),
      "{\"_CG\":{\"meta\":{},\"nodes\":{\"0\":{\"callees\":{},\"functionName\":\"main\",\"hasBody\":true,\"meta\":{},"
      "\"origin\":\"main.cpp\"}}}"
      ",\"_MetaCG\":{\"generator\":{\"name\":\"Test\",\"sha\":\"TestSha\",\"version\":\"0.1\"},\"version\":\"4.0\"}}");
}

TEST_F(V4MCGWriterTest, WritePointer) {
  auto& mcgm = metacg::graph::MCGManager::get();
  mcgm.addToManagedGraphs("newGraph", std::make_unique<metacg::Callgraph>());
  const auto& cg = mcgm.getCallgraph();
  cg->insert("main", "main.cpp");
  cg->getMain()->setHasBody(true);

  const std::string generatorName = "Test";
  const metacg::MCGFileInfo mcgFileInfo = {{4, 0}, {generatorName, 0, 1, "TestSha"}};
  metacg::io::VersionFourMCGWriter mcgWriter(mcgFileInfo);
  metacg::io::JsonSink jsonSink;
  mcgWriter.write(mcgm.getCallgraph("emptyGraph"), jsonSink);
  EXPECT_EQ(jsonSink.getJson().dump(),
            "{\"_CG\":{\"meta\":{},\"nodes\":{}},"
            "\"_MetaCG\":{\"generator\":{\"name\":\"Test\",\"sha\":\"TestSha\",\"version\":\"0.1\"},\"version\":\"4."
            "0\"}}");

  mcgWriter.write(mcgm.getCallgraph("newGraph"), jsonSink);
  EXPECT_EQ(
      jsonSink.getJson().dump(),
      "{\"_CG\":{\"meta\":{},\"nodes\":{\"0\":{\"callees\":{},\"functionName\":\"main\",\"hasBody\":true,\"meta\":{},"
      "\"origin\":\"main.cpp\"}}}"
      ",\"_MetaCG\":{\"generator\":{\"name\":\"Test\",\"sha\":\"TestSha\",\"version\":\"0.1\"},\"version\":\"4.0\"}}");
}

TEST_F(V4MCGWriterTest, SwitchBeforeWrite) {
  auto& mcgm = metacg::graph::MCGManager::get();
  mcgm.addToManagedGraphs("newGraph", std::make_unique<metacg::Callgraph>());
  const auto& cg = mcgm.getCallgraph();
  cg->insert("main", "main.cpp");
  cg->getMain()->setHasBody(true);

  const std::string generatorName = "Test";
  const metacg::MCGFileInfo mcgFileInfo = {{4, 0}, {generatorName, 0, 1, "TestSha"}};
  metacg::io::VersionFourMCGWriter mcgWriter(mcgFileInfo);
  metacg::io::JsonSink jsonSink;

  mcgm.setActive("emptyGraph");
  mcgm.setActive("newGraph");
  mcgm.setActive("emptyGraph");

  mcgWriter.writeActiveGraph(jsonSink);
  EXPECT_EQ(jsonSink.getJson().dump(),
            "{\"_CG\":{\"meta\":{},\"nodes\":{}},"
            "\"_MetaCG\":{\"generator\":{\"name\":\"Test\",\"sha\":\"TestSha\",\"version\":\"0.1\"},\"version\":\"4."
            "0\"}}");

  mcgm.setActive("newGraph");

  mcgWriter.writeActiveGraph(jsonSink);
  EXPECT_EQ(
      jsonSink.getJson().dump(),
      "{\"_CG\":{\"meta\":{},\"nodes\":{\"0\":{\"callees\":{},\"functionName\":\"main\",\"hasBody\":true,\"meta\":{},"
      "\"origin\":\"main.cpp\"}}}"
      ",\"_MetaCG\":{\"generator\":{\"name\":\"Test\",\"sha\":\"TestSha\",\"version\":\"0.1\"},\"version\":\"4.0\"}}");
}
static void removeLogDate(std::string& log) { log.erase(0, log.find_first_of(']') + 2); }

TEST_F(V4MCGWriterTest, UnserializableNodeMetadataWarnsOnlyOnNull) {
  auto& mcgm = metacg::graph::MCGManager::get();
  const auto& cg = mcgm.getCallgraph();
  auto& main = cg->insert("main", "main.cpp");
  main.setHasBody(true);

  main.addMetaData(std::make_unique<ExportEmptyTestMD<true>>());
  main.addMetaData(std::make_unique<ExportEmptyTestMD<false>>());

  const std::string generatorName = "Test";
  const metacg::MCGFileInfo mcgFileInfo = {{4, 0}, {generatorName, 0, 1, "TestSha"}};
  metacg::io::VersionFourMCGWriter mcgWriter(mcgFileInfo);
  metacg::io::JsonSink jsonSink;

  metacg::loggerutil::disableColor();
  testing::internal::CaptureStdout();
  mcgWriter.writeActiveGraph(jsonSink);
  std::string output = testing::internal::GetCapturedStdout();
  removeLogDate(output);

  // Only the null-returning metadata should trigger a warning, the empty one is skipped silently
  EXPECT_EQ(output, "[console] [warning] Could not serialize metadata of type NullExportTestMD in node main\n");

  // Neither metadata is attached to the output
  EXPECT_EQ(
      jsonSink.getJson().dump(),
      "{\"_CG\":{\"meta\":{},\"nodes\":{\"0\":{\"callees\":{},\"functionName\":\"main\",\"hasBody\":true,\"meta\":{},"
      "\"origin\":\"main.cpp\"}}},"
      "\"_MetaCG\":{\"generator\":{\"name\":\"Test\",\"sha\":\"TestSha\",\"version\":\"0.1\"},\"version\":\"4.0\"}}");
}

TEST_F(V4MCGWriterTest, UnserializableGlobalMetadataWarnsOnlyOnNull) {
  auto& mcgm = metacg::graph::MCGManager::get();
  const auto& cg = mcgm.getCallgraph();
  auto& main = cg->insert("main", "main.cpp");
  main.setHasBody(true);

  cg->addMetaData(std::make_unique<ExportEmptyTestMD<true>>());
  cg->addMetaData(std::make_unique<ExportEmptyTestMD<false>>());

  const std::string generatorName = "Test";
  const metacg::MCGFileInfo mcgFileInfo = {{4, 0}, {generatorName, 0, 1, "TestSha"}};
  metacg::io::VersionFourMCGWriter mcgWriter(mcgFileInfo);
  metacg::io::JsonSink jsonSink;

  metacg::loggerutil::disableColor();
  testing::internal::CaptureStdout();
  mcgWriter.writeActiveGraph(jsonSink);
  std::string output = testing::internal::GetCapturedStdout();
  removeLogDate(output);

  // Only the null-returning metadata should trigger a warning, the empty one is skipped silently
  EXPECT_EQ(output, "[console] [warning] Could not serialize global metadata of type NullExportTestMD\n");

  // Neither metadata is attached to the output
  EXPECT_EQ(
      jsonSink.getJson().dump(),
      "{\"_CG\":{\"meta\":{},\"nodes\":{\"0\":{\"callees\":{},\"functionName\":\"main\",\"hasBody\":true,\"meta\":{},"
      "\"origin\":\"main.cpp\"}}},"
      "\"_MetaCG\":{\"generator\":{\"name\":\"Test\",\"sha\":\"TestSha\",\"version\":\"0.1\"},\"version\":\"4.0\"}}");
}

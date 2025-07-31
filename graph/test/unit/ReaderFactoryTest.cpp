
/**
 * File: ReaderFactoryTest.cpp
 * License: Part of the metacg project. Licensed under BSD 3 clause license. See LICENSE.txt file at
 * https://github.com/tudasc/metacg/LICENSE.txt
 */

#include "LoggerUtil.h"
#include "MCGManager.h"
#include "io/VersionThreeMCGReader.h"
#include "io/VersionTwoMCGReader.h"
#include "nlohmann/json.hpp"

#include "gtest/gtest.h"

class MCGReaderFactoryTest : public ::testing::Test {
 protected:
  void SetUp() override {
    auto& mcgm = metacg::graph::MCGManager::get();
    mcgm.resetManager();
  }
};

TEST(MCGReaderFactoryTest, V3Reader) {
  const nlohmann::json j =
      "{\"_CG\":"
      "{\n"
      "    \"edges\": [],\n"
      "    \"nodes\": []\n"
      "  }"
      ",\"_MetaCG\":{"
      "   \"generator\":{\"name\":\"Test\",\"sha\":\"TestSha\",\"version\":\"0.1\"},"
      "    \"version\":\"3.0\"}"
      "}"_json;

  auto& mcgm = metacg::graph::MCGManager::get();
  mcgm.resetManager();

  metacg::io::JsonSource source(j);

  auto reader = metacg::io::createReader(source);
  ASSERT_TRUE(reader);

  mcgm.addToManagedGraphs("newCallgraph", reader->read());
  const metacg::Callgraph& graph = *mcgm.getCallgraph();
  ASSERT_EQ(graph.size(), 0);
}

TEST(MCGReaderFactoryTest, V2Reader) {
  const nlohmann::json j =
      "{\n"
      "   \"_CG\":{},\n"
      "   \"_MetaCG\":{\n"
      "      \"generator\":{\n"
      "         \"name\":\"Test\",\n"
      "         \"sha\":\"TestSha\",\n"
      "         \"version\":\"0.1\"\n"
      "      },\n"
      "      \"version\":\"2.0\"\n"
      "   }\n"
      "}"_json;
  auto& mcgm = metacg::graph::MCGManager::get();
  mcgm.resetManager();

  metacg::io::JsonSource source(j);

  auto reader = metacg::io::createReader(source);
  ASSERT_TRUE(reader);

  mcgm.addToManagedGraphs("newCallgraph", reader->read());
  const metacg::Callgraph& graph = *mcgm.getCallgraph();
  ASSERT_EQ(graph.size(), 0);
}

TEST(MCGReaderFactoryTest, V1Format) {
  const nlohmann::json j =
      "{\n"
      "  \"main\": {\n"
      "    \"callees\": [],\n"
      "    \"doesOverride\": false,\n"
      "    \"hasBody\": true,\n"
      "    \"isVirtual\": false,\n"
      "    \"numStatements\": 2,\n"
      "    \"overriddenBy\": [],\n"
      "    \"overriddenFunctions\": [],\n"
      "    \"parents\": []\n"
      "  }\n"
      "}"_json;

  metacg::io::JsonSource source(j);
  ASSERT_EQ(source.getFormatVersion(), "unknown");

  auto reader = metacg::io::createReader(source);
  ASSERT_FALSE(reader);
}

TEST(MCGReaderFactoryTest, UnsupportedVersion) {
  const nlohmann::json j =
      "{\n"
      "   \"_MetaCG\":{\n"
      "      \"version\":\"4.0\"\n"
      "   }\n"
      "}"_json;
  auto& mcgm = metacg::graph::MCGManager::get();
  mcgm.resetManager();

  metacg::io::JsonSource source(j);

  ASSERT_EQ(source.getFormatVersion(), "4.0");

  auto reader = metacg::io::createReader(source);
  ASSERT_FALSE(reader);
}

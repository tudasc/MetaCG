
/**
 * File: VersionFourMCGReaderTest.cpp
 * License: Part of the metacg project. Licensed under BSD 3 clause license. See LICENSE.txt file at
 * https://github.com/tudasc/metacg/LICENSE.txt
 */

#include "io/VersionFourMCGReader.h"
#include "LoggerUtil.h"
#include "MCGManager.h"
#include "TestMD.h"

#include "nlohmann/json.hpp"
#include "gtest/gtest.h"

using namespace metacg;
using json = nlohmann::json;

class V4MCGReaderTest : public ::testing::Test {
 protected:
  void SetUp() override {
    auto& mcgm = metacg::graph::MCGManager::get();
    mcgm.resetManager();
  }
};

TEST(V4MCGReaderTest, EmptyJSON) {
  const json j =
      "{\n"
      "    \"_CG\": {},\n"
      "    \"_MetaCG\": {\n"
      "        \"generator\": {\n"
      "            \"name\": \"MetaCG\",\n"
      "            \"sha\": \"4ad73ebf7a108aa388d232ee4824bada13feaa4c\",\n"
      "            \"version\": \"0.7\"\n"
      "        },\n"
      "        \"version\": \"4.0\"\n"
      "    }\n"
      "}"_json;

  auto& mcgm = metacg::graph::MCGManager::get();
  mcgm.resetManager();

  metacg::io::JsonSource source(j);
  metacg::io::VersionFourMetaCGReader reader(source);
  mcgm.addToManagedGraphs("newCallgraph", reader.read());
  const Callgraph& graph = *mcgm.getCallgraph();
  ASSERT_EQ(graph.size(), 0);
}

TEST(V4MCGReaderTest, SingleNode) {
  const json j =
      "{\n"
      "    \"_CG\": {\n"
      "        \"0\": {\n"
      "            \"callees\": null,\n"
      "            \"functionName\": \"main\",\n"
      "            \"hasBody\": true,\n"
      "            \"meta\": null,\n"
      "            \"origin\": \"main.cpp\"\n"
      "        }\n"
      "    },\n"
      "    \"_MetaCG\": {\n"
      "        \"generator\": {\n"
      "            \"name\": \"MetaCG\",\n"
      "            \"sha\": \"4ad73ebf7a108aa388d232ee4824bada13feaa4c\",\n"
      "            \"version\": \"0.7\"\n"
      "        },\n"
      "        \"version\": \"4.0\"\n"
      "    }\n"
      "}"_json;

  auto& mcgm = metacg::graph::MCGManager::get();
  mcgm.resetManager();

  metacg::io::JsonSource source(j);
  metacg::io::VersionFourMetaCGReader reader(source);
  mcgm.addToManagedGraphs("newCallgraph", reader.read());
  Callgraph& graph = *mcgm.getCallgraph();
  EXPECT_EQ(graph.size(), 1);
  EXPECT_NE(graph.getMain(), nullptr);
  EXPECT_EQ(graph.getMain()->getFunctionName(), "main");

  EXPECT_EQ(graph.getEdges().size(), 0);
  EXPECT_EQ(graph.getMain()->getMetaDataContainer().size(), 0);
}

TEST(V4MCGReaderTest, NodesAndSingleEdge) {
  const json j =
      "{\n"
      "    \"_CG\": {\n"
      "        \"0\": {\n"
      "            \"callees\": {\n"
      "                \"2\": null\n"
      "            },\n"
      "            \"functionName\": \"Function1\",\n"
      "            \"hasBody\": true,\n"
      "            \"meta\": null,\n"
      "            \"origin\": \"function1and2.cpp\"\n"
      "        },\n"
      "        \"1\": {\n"
      "            \"callees\": null,\n"
      "            \"functionName\": \"Function2\",\n"
      "            \"hasBody\": false,\n"
      "            \"meta\": null,\n"
      "            \"origin\": \"function1and2.cpp\"\n"
      "        },\n"
      "        \"2\": {\n"
      "            \"callees\": null,\n"
      "            \"functionName\": \"Function3\",\n"
      "            \"hasBody\": true,\n"
      "            \"meta\": null,\n"
      "            \"origin\": \"function3.cpp\"\n"
      "        }\n"
      "    },\n"
      "    \"_MetaCG\": {\n"
      "        \"generator\": {\n"
      "            \"name\": \"MetaCG\",\n"
      "            \"sha\": \"4ad73ebf7a108aa388d232ee4824bada13feaa4c\",\n"
      "            \"version\": \"0.7\"\n"
      "        },\n"
      "        \"version\": \"4.0\"\n"
      "    }\n"
      "}"_json;

  auto& mcgm = metacg::graph::MCGManager::get();
  mcgm.resetManager();
  metacg::io::JsonSource source(j);
  metacg::io::VersionFourMetaCGReader reader(source);
  mcgm.addToManagedGraphs("newCallgraph", reader.read());

  const Callgraph& graph = *mcgm.getCallgraph();
  EXPECT_EQ(graph.size(), 3);

  metacg::CgNode* function1 = &graph.getSingleNode("Function1");
  ASSERT_NE(function1, nullptr);
  EXPECT_EQ(function1->getFunctionName(), "Function1");
  EXPECT_EQ(function1->getHasBody(), true);
  EXPECT_EQ(function1->getOrigin(), "function1and2.cpp");

  metacg::CgNode* function2 = &graph.getSingleNode("Function2");
  ASSERT_NE(function2, nullptr);
  EXPECT_EQ(function2->getFunctionName(), "Function2");
  EXPECT_EQ(function2->getHasBody(), false);
  EXPECT_EQ(function2->getOrigin(), "function1and2.cpp");

  EXPECT_EQ(function1->getOrigin(), function2->getOrigin());

  metacg::CgNode* function3 = &graph.getSingleNode("Function3");
  ASSERT_NE(function3, nullptr);
  EXPECT_EQ(function3->getFunctionName(), "Function3");
  EXPECT_EQ(function3->getHasBody(), true);
  EXPECT_EQ(function3->getOrigin(), "function3.cpp");

  EXPECT_FALSE(graph.existsAnyEdge("Function1", "Function1"));
  EXPECT_FALSE(graph.existsAnyEdge("Function1", "Function2"));
  EXPECT_TRUE(graph.existsAnyEdge("Function1", "Function3"));

  EXPECT_FALSE(graph.existsAnyEdge("Function2", "Function1"));
  EXPECT_FALSE(graph.existsAnyEdge("Function2", "Function2"));
  EXPECT_FALSE(graph.existsAnyEdge("Function2", "Function3"));

  EXPECT_FALSE(graph.existsAnyEdge("Function3", "Function1"));
  EXPECT_FALSE(graph.existsAnyEdge("Function3", "Function2"));
  EXPECT_FALSE(graph.existsAnyEdge("Function3", "Function3"));

  EXPECT_EQ(function1->getMetaDataContainer().size(), 0);
}

TEST(V4MCGReaderTest, NodeMetaData) {
  const json j =
      "{\n"
      "    \"_CG\": {\n"
      "        \"0\": {\n"
      "            \"callees\": null,\n"
      "            \"functionName\": \"main\",\n"
      "            \"hasBody\": true,\n"
      "            \"meta\": {\n"
      "                \"TestMetaDataV4\": {\n"
      "                    \"stored_double\": 13.37,\n"
      "                    \"stored_int\": 42,\n"
      "                    \"stored_string\": \"Test\"\n"
      "                }\n"
      "            },\n"
      "            \"origin\": \"main.cpp\"\n"
      "        }\n"
      "    },\n"
      "    \"_MetaCG\": {\n"
      "        \"generator\": {\n"
      "            \"name\": \"MetaCG\",\n"
      "            \"sha\": \"4ad73ebf7a108aa388d232ee4824bada13feaa4c\",\n"
      "            \"version\": \"0.7\"\n"
      "        },\n"
      "        \"version\": \"4.0\"\n"
      "    }\n"
      "}"_json;

  auto& mcgm = metacg::graph::MCGManager::get();
  mcgm.resetManager();
  metacg::io::JsonSource source(j);
  metacg::io::VersionFourMetaCGReader reader(source);
  mcgm.addToManagedGraphs("newCallgraph", reader.read());
  const Callgraph& graph = *mcgm.getCallgraph();

  EXPECT_EQ(graph.getSingleNode("main").getMetaDataContainer().size(), 1);
  EXPECT_NE(graph.getSingleNode("main").get<TestMetaDataV4>(), nullptr);
}

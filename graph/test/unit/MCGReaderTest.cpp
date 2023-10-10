/**
 * File: DotIOTest.cpp
 * License: Part of the MetaCG project. Licensed under BSD 3 clause license. See LICENSE.txt file at
 * https://github.com/tudasc/metacg/LICENSE.txt
 */

#include "LoggerUtil.h"
#include "gtest/gtest.h"

#include "MCGManager.h"
#include "MCGReader.h"

class V2MCGReaderTest : public ::testing::Test {
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

TEST_F(V2MCGReaderTest, NullCG) {
  nlohmann::json j;
  auto &mcgm = metacg::graph::MCGManager::get();
  metacg::io::JsonSource jsonSource(j);
  metacg::io::VersionTwoMetaCGReader mcgReader(jsonSource);
  try {
    mcgReader.read(mcgm);
    EXPECT_TRUE(false);  // should not reach here
  } catch (std::exception &e) {
    EXPECT_TRUE(strcmp(e.what(), "JSON source did not contain any data.") == 0);
  }
}

TEST_F(V2MCGReaderTest, BrokenMetaInformation) {
  nlohmann::json j =
      "{\n"
      "   \"_CG\":null,\n"
      "   \"_MetaCG\": null\n"
      "}"_json;
  metacg::io::JsonSource jsonSource(j);
  metacg::io::VersionTwoMetaCGReader mcgReader(jsonSource);
  auto &mcgm = metacg::graph::MCGManager::get();

  try {
    mcgReader.read(mcgm);
    EXPECT_TRUE(false);  // should not reach here
  } catch (std::exception &e) {
    EXPECT_TRUE(strcmp(e.what(), "Could not read version info from metacg file.") == 0);
  }
}

TEST_F(V2MCGReaderTest, WrongVersionInformation) {
  nlohmann::json j =
      "{\n"
      "         \"_CG\": null,\n"
      "         \"_MetaCG\":{\n"
      "            \"generator\":{\n"
      "               \"name\":\"Test\",\n"
      "               \"sha\":\"TestSha\",\n"
      "               \"version\":\"0.1\"\n"
      "            },\n"
      "            \"version\":\"3.0\"\n"
      "         }\n"
      "      }"_json;
  metacg::io::JsonSource jsonSource(j);
  metacg::io::VersionTwoMetaCGReader mcgReader(jsonSource);
  auto &mcgm = metacg::graph::MCGManager::get();

  try {
    mcgReader.read(mcgm);
    EXPECT_TRUE(false);  // should not reach here
  } catch (std::exception &e) {
    EXPECT_TRUE(strcmp(e.what(), "File is of version 3.0, this reader handles version 2.x") == 0);
  }
}

TEST_F(V2MCGReaderTest, BrokenCG) {
  nlohmann::json j =
      "{\n"
      "         \"_CG\": null,\n"
      "         \"_MetaCG\":{\n"
      "            \"generator\":{\n"
      "               \"name\":\"Test\",\n"
      "               \"sha\":\"TestSha\",\n"
      "               \"version\":\"0.1\"\n"
      "            },\n"
      "            \"version\":\"2.0\"\n"
      "         }\n"
      "      }"_json;

  metacg::io::JsonSource jsonSource(j);
  metacg::io::VersionTwoMetaCGReader mcgReader(jsonSource);
  auto &mcgm = metacg::graph::MCGManager::get();
  try {
    mcgReader.read(mcgm);
    EXPECT_TRUE(false);  // should not reach here
  } catch (std::exception &e) {
    EXPECT_TRUE(strcmp(e.what(), "The call graph in the metacg file was not found or null.") == 0);
  }
}

TEST_F(V2MCGReaderTest, EmptyCGRead) {
  nlohmann::json j =
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
  metacg::io::JsonSource jsonSource(j);
  metacg::io::VersionTwoMetaCGReader mcgReader(jsonSource);
  auto &mcgm = metacg::graph::MCGManager::get();
  mcgReader.read(mcgm);
  EXPECT_EQ(mcgm.graphs_size(), 1);
  const auto &cg = mcgm.getCallgraph();
  EXPECT_EQ(cg->size(), 0);
}

TEST_F(V2MCGReaderTest, OneNodeCGRead) {
  nlohmann::json j =
      "{\n"
      "   \"_CG\":{\n"
      "      \"main\":{\n"
      "         \"callees\":[],\n"
      "         \"callers\":[],\n"
      "         \"doesOverride\":false,\n"
      "         \"hasBody\":true,\n"
      "         \"isVirtual\":false,\n"
      "         \"meta\":null,\n"
      "         \"overriddenBy\":[],\n"
      "         \"overrides\":[]\n"
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
  metacg::io::JsonSource jsonSource(j);
  metacg::io::VersionTwoMetaCGReader mcgReader(jsonSource);
  auto &mcgm = metacg::graph::MCGManager::get();
  mcgReader.read(mcgm);
  EXPECT_EQ(mcgm.graphs_size(), 1);
  const auto &cg = mcgm.getCallgraph();
  EXPECT_EQ(cg->size(), 1);
  EXPECT_TRUE(cg->hasNode("main"));
  EXPECT_TRUE(cg->getMain() != nullptr);
  EXPECT_TRUE(cg->getMain()->getFunctionName() == "main");
  EXPECT_TRUE(cg->getMain()->getId() == std::hash<std::string>()("main"));
  EXPECT_TRUE(cg->getLastSearchedNode()->getFunctionName() == "main");
  EXPECT_TRUE(cg->getMain()->getHasBody());
  EXPECT_FALSE(cg->getMain()->isVirtual());
  EXPECT_TRUE(cg->getMain()->getMetaDataContainer().empty());
  EXPECT_TRUE(cg->getCallees("main").empty());
  EXPECT_TRUE(cg->getCallers("main").empty());
}

TEST_F(V2MCGReaderTest, TwoNodeCGRead) {
  nlohmann::json j =
      "{\n"
      "   \"_CG\":{\n"
      "      \"main\":{\n"
      "         \"callees\":[],\n"
      "         \"callers\":[],\n"
      "         \"doesOverride\":false,\n"
      "         \"hasBody\":true,\n"
      "         \"isVirtual\":false,\n"
      "         \"meta\":null,\n"
      "         \"overriddenBy\":[],\n"
      "         \"overrides\":[]\n"
      "      },\n"
      "      \"foo\":{\n"
      "         \"callees\":[],\n"
      "         \"callers\":[],\n"
      "         \"doesOverride\":false,\n"
      "         \"hasBody\":false,\n"
      "         \"isVirtual\":false,\n"
      "         \"meta\":null,\n"
      "         \"overriddenBy\":[],\n"
      "         \"overrides\":[]\n"
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

  metacg::io::JsonSource jsonSource(j);
  metacg::io::VersionTwoMetaCGReader mcgReader(jsonSource);
  auto &mcgm = metacg::graph::MCGManager::get();
  mcgReader.read(mcgm);
  EXPECT_EQ(mcgm.graphs_size(), 1);
  const auto &cg = mcgm.getCallgraph();
  EXPECT_EQ(cg->size(), 2);
  EXPECT_TRUE(cg->hasNode("main"));
  EXPECT_TRUE(cg->getMain() != nullptr);
  EXPECT_TRUE(cg->getMain()->getFunctionName() == "main");
  EXPECT_TRUE(cg->getMain()->getId() == std::hash<std::string>()("main"));
  EXPECT_TRUE(cg->getLastSearchedNode()->getFunctionName() == "main");
  EXPECT_TRUE(cg->getMain()->getHasBody());
  EXPECT_FALSE(cg->getMain()->isVirtual());
  EXPECT_TRUE(cg->getMain()->getMetaDataContainer().empty());
  EXPECT_TRUE(cg->getCallees("main").empty());
  EXPECT_TRUE(cg->getCallers("main").empty());

  EXPECT_TRUE(cg->hasNode("foo"));
  EXPECT_TRUE(cg->getNode("foo")->getId() == std::hash<std::string>()("foo"));
  EXPECT_TRUE(cg->getLastSearchedNode()->getFunctionName() == "foo");
  EXPECT_FALSE(cg->getNode("foo")->getHasBody());
  EXPECT_FALSE(cg->getNode("foo")->isVirtual());
  EXPECT_TRUE(cg->getNode("foo")->getMetaDataContainer().empty());
  EXPECT_TRUE(cg->getCallees("foo").empty());
  EXPECT_TRUE(cg->getCallers("foo").empty());
}

TEST_F(V2MCGReaderTest, TwoNodeOneEdgeCGRead) {
  nlohmann::json j =
      "{\n"
      "   \"_CG\":{\n"
      "      \"main\":{\n"
      "         \"callees\":[\"foo\"],\n"
      "         \"callers\":[],\n"
      "         \"doesOverride\":false,\n"
      "         \"hasBody\":true,\n"
      "         \"isVirtual\":false,\n"
      "         \"meta\":null,\n"
      "         \"overriddenBy\":[],\n"
      "         \"overrides\":[]\n"
      "      },\n"
      "      \"foo\":{\n"
      "         \"callees\":[],\n"
      "         \"callers\":[\"main\"],\n"
      "         \"doesOverride\":false,\n"
      "         \"hasBody\":false,\n"
      "         \"isVirtual\":false,\n"
      "         \"meta\":null,\n"
      "         \"overriddenBy\":[],\n"
      "         \"overrides\":[]\n"
      "      }\n"
      "   },\n"
      "   \"_MetaCG\":{\n"
      "      \"generator\":{\n"
      "         \"name\":\"GenCC\",\n"
      "         \"sha\":\"NO_GIT_SHA_AVAILABLE\",\n"
      "         \"version\":\"0.1\"\n"
      "      },\n"
      "      \"version\":\"2.0\"\n"
      "   }"
      "}"_json;

  metacg::io::JsonSource jsonSource(j);
  metacg::io::VersionTwoMetaCGReader mcgReader(jsonSource);
  auto &mcgm = metacg::graph::MCGManager::get();
  mcgReader.read(mcgm);
  EXPECT_EQ(mcgm.graphs_size(), 1);
  const auto &cg = mcgm.getCallgraph();
  EXPECT_EQ(cg->size(), 2);
  EXPECT_TRUE(cg->hasNode("main"));
  EXPECT_TRUE(cg->getMain() != nullptr);
  EXPECT_TRUE(cg->getMain()->getFunctionName() == "main");
  EXPECT_TRUE(cg->getMain()->getId() == std::hash<std::string>()("main"));
  EXPECT_TRUE(cg->getLastSearchedNode()->getFunctionName() == "main");
  EXPECT_TRUE(cg->getMain()->getHasBody());
  EXPECT_FALSE(cg->getMain()->isVirtual());
  EXPECT_TRUE(cg->getMain()->getMetaDataContainer().empty());
  EXPECT_TRUE(cg->getCallees("main").size() == 1);
  for (const auto &callee : cg->getCallees("main")) {
    EXPECT_TRUE(callee->getFunctionName() == "foo");
  }
  EXPECT_TRUE(cg->getCallers("main").empty());

  EXPECT_TRUE(cg->hasNode("foo"));
  EXPECT_TRUE(cg->getNode("foo")->getId() == std::hash<std::string>()("foo"));
  EXPECT_TRUE(cg->getLastSearchedNode()->getFunctionName() == "foo");
  EXPECT_FALSE(cg->getNode("foo")->getHasBody());
  EXPECT_FALSE(cg->getNode("foo")->isVirtual());
  EXPECT_TRUE(cg->getNode("foo")->getMetaDataContainer().empty());
  EXPECT_TRUE(cg->getCallees("foo").empty());
  EXPECT_TRUE(cg->getCallers("foo").size() == 1);
  for (const auto &caller : cg->getCallers("foo")) {
    EXPECT_TRUE(caller->getFunctionName() == "main");
  }
}

TEST_F(V2MCGReaderTest, ThreeNodeOneEdgeCGRead) {
  nlohmann::json j =
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
      "         \"meta\":null,\n"
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
      "         \"isVirtual\":false,\n"
      "         \"meta\":null,\n"
      "         \"overriddenBy\":[],\n"
      "         \"overrides\":[]\n"
      "      },\n"
      "      \"bar\":{\n"
      "         \"callees\":[],\n"
      "         \"callers\":[],\n"
      "         \"doesOverride\":false,\n"
      "         \"hasBody\":true,\n"
      "         \"isVirtual\":true,\n"
      "         \"meta\":null,\n"
      "         \"overriddenBy\":[],\n"
      "         \"overrides\":[]\n"
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

  metacg::io::JsonSource jsonSource(j);
  metacg::io::VersionTwoMetaCGReader mcgReader(jsonSource);
  auto &mcgm = metacg::graph::MCGManager::get();
  mcgReader.read(mcgm);
  EXPECT_EQ(mcgm.graphs_size(), 1);
  const auto &cg = mcgm.getCallgraph();
  EXPECT_EQ(cg->size(), 3);
  EXPECT_TRUE(cg->hasNode("main"));
  EXPECT_TRUE(cg->getMain() != nullptr);
  EXPECT_TRUE(cg->getMain()->getFunctionName() == "main");
  EXPECT_TRUE(cg->getMain()->getId() == std::hash<std::string>()("main"));
  EXPECT_TRUE(cg->getLastSearchedNode()->getFunctionName() == "main");
  EXPECT_TRUE(cg->getMain()->getHasBody());
  EXPECT_FALSE(cg->getMain()->isVirtual());
  EXPECT_TRUE(cg->getMain()->getMetaDataContainer().empty());
  EXPECT_TRUE(cg->getCallees("main").size() == 1);
  for (const auto &callee : cg->getCallees("main")) {
    EXPECT_TRUE(callee->getFunctionName() == "foo");
  }
  EXPECT_TRUE(cg->getCallers("main").empty());

  EXPECT_TRUE(cg->hasNode("foo"));
  EXPECT_TRUE(cg->getNode("foo")->getId() == std::hash<std::string>()("foo"));
  EXPECT_TRUE(cg->getLastSearchedNode()->getFunctionName() == "foo");
  EXPECT_FALSE(cg->getNode("foo")->getHasBody());
  EXPECT_FALSE(cg->getNode("foo")->isVirtual());
  EXPECT_TRUE(cg->getNode("foo")->getMetaDataContainer().empty());
  EXPECT_TRUE(cg->getCallees("foo").empty());
  EXPECT_TRUE(cg->getCallers("foo").size() == 1);
  for (const auto &caller : cg->getCallers("foo")) {
    EXPECT_TRUE(caller->getFunctionName() == "main");
  }

  EXPECT_TRUE(cg->hasNode("bar"));
  EXPECT_TRUE(cg->getNode("bar")->getId() == std::hash<std::string>()("bar"));
  EXPECT_TRUE(cg->getLastSearchedNode()->getFunctionName() == "bar");
  EXPECT_TRUE(cg->getNode("bar")->getHasBody());
  EXPECT_TRUE(cg->getNode("bar")->isVirtual());
  EXPECT_TRUE(cg->getNode("bar")->getMetaDataContainer().empty());
  EXPECT_TRUE(cg->getCallees("bar").empty());
  EXPECT_TRUE(cg->getCallers("bar").empty());
}

TEST_F(V2MCGReaderTest, MetadataCGRead) {
  nlohmann::json j =
      "{\n"
      "   \"_CG\":{\n"
      "      \"main\":{\n"
      "         \"callees\":[],\n"
      "         \"callers\":[],\n"
      "         \"doesOverride\":false,\n"
      "         \"hasBody\":true,\n"
      "         \"isVirtual\":false,\n"
      "         \"meta\": {\n"
      "           \"TestMetaData\": {\n"
      "                \"metadataString\":\"Test\",\n"
      "                \"metadataInt\": 1337,\n"
      "                \"metadataFloat\": 3.1415\n"
      "           }\n"
      "         },\n"
      "         \"overriddenBy\":[],\n"
      "         \"overrides\":[]\n"
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

  metacg::io::JsonSource jsonSource(j);
  metacg::io::VersionTwoMetaCGReader mcgReader(jsonSource);
  auto &mcgm = metacg::graph::MCGManager::get();
  mcgReader.read(mcgm);
  EXPECT_EQ(mcgm.graphs_size(), 1);
  const auto &cg = mcgm.getCallgraph();
  EXPECT_EQ(cg->size(), 1);
  EXPECT_TRUE(cg->hasNode("main"));
  EXPECT_TRUE(cg->getMain() != nullptr);
  EXPECT_TRUE(cg->getMain()->getFunctionName() == "main");
  EXPECT_TRUE(cg->getMain()->getId() == std::hash<std::string>()("main"));
  EXPECT_TRUE(cg->getLastSearchedNode()->getFunctionName() == "main");
  EXPECT_TRUE(cg->getMain()->getHasBody());
  EXPECT_FALSE(cg->getMain()->isVirtual());
  EXPECT_TRUE(cg->getMain()->getMetaDataContainer().size() == 1);
  EXPECT_TRUE(cg->getMain()->has<TestMetaData>());
  EXPECT_TRUE(cg->getMain()->get<TestMetaData>()->metadataString == "Test");
  EXPECT_TRUE(cg->getMain()->get<TestMetaData>()->metadataInt == 1337);
  EXPECT_TRUE(cg->getMain()->get<TestMetaData>()->metadataFloat == 3.1415f);
  EXPECT_TRUE(cg->getCallees("main").empty());
  EXPECT_TRUE(cg->getCallers("main").empty());
}

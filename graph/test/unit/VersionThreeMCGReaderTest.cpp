//
///**
// * File: VersionThreeMCGReaderTest.cpp
// * License: Part of the metacg project. Licensed under BSD 3 clause license. See LICENSE.txt file at
// * https://github.com/tudasc/metacg/LICENSE.txt
// */
//
//#include "LoggerUtil.h"
//#include "MCGManager.h"
//
//#include "gtest/gtest.h"
//
//#include "nlohmann/json.hpp"
//
//#include "io/VersionFourMCGReader.h"
//
//using namespace metacg;
//using json = nlohmann::json;
//
//struct TestMetaDataV3 final : metacg::MetaData::Registrar<TestMetaDataV3> {
//  static constexpr const char* key = "TestMetaDataV3";
//
//  explicit TestMetaDataV3(const nlohmann::json& j) {
//    stored_int = j.at("stored_int");
//    stored_double = j.at("stored_double");
//    stored_string = j.at("stored_string");
//  }
//
//  explicit TestMetaDataV3(int storeInt, double storeDouble, const std::string& storeString) {
//    stored_int = storeInt;
//    stored_double = storeDouble;
//    stored_string = storeString;
//  }
//
// private:
//  TestMetaDataV3(const TestMetaDataV3& other)
//      : stored_int(other.stored_int), stored_double(other.stored_double), stored_string(other.stored_string) {}
//
// public:
//  nlohmann::json toJson() const final {
//    return {{"stored_int", stored_int}, {"stored_double", stored_double}, {"stored_string", stored_string}};
//  };
//
//  const char* getKey() const override { return key; }
//
//  void merge(const MetaData& toMerge, const MergeAction&, const GraphMapping&) final {
//    if (std::strcmp(toMerge.getKey(), getKey()) != 0) {
//      metacg::MCGLogger::instance().getErrConsole()->error(
//          "The MetaData which was tried to merge with TestMetaDataV3 was of a different MetaData type");
//      abort();
//    }
//
//    // const TestMetaDataV3* toMergeDerived = static_cast<const TestMetaDataV3*>(&toMerge);
//  }
//
//  MetaData* clone() const final { return new TestMetaDataV3(*this); }
//
// private:
//  int stored_int;
//  double stored_double;
//  std::string stored_string;
//};
//
//class V3MCGReaderTest : public ::testing::Test {
// protected:
//  void SetUp() override {
//    auto& mcgm = metacg::graph::MCGManager::get();
//    mcgm.resetManager();
//  }
//};
//
//TEST(V3MCGReaderTest, EmptyJSON) {
//  const json j =
//      "{\"_CG\":"
//      "{\n"
//      "    \"edges\": [],\n"
//      "    \"nodes\": []\n"
//      "  }"
//      ",\"_MetaCG\":{"
//      "   \"generator\":{\"name\":\"Test\",\"sha\":\"TestSha\",\"version\":\"0.1\"},"
//      "    \"version\":\"3.0\"}"
//      "}"_json;
//
//  auto& mcgm = metacg::graph::MCGManager::get();
//  mcgm.resetManager();
//
//  metacg::io::JsonSource source(j);
//  metacg::io::VersionThreeMetaCGReader reader(source);
//  mcgm.addToManagedGraphs("newCallgraph", reader.read());
//  const Callgraph& graph = *mcgm.getCallgraph();
//  ASSERT_EQ(graph.size(), 0);
//}
//
//TEST(V3MCGReaderTest, SingleNode) {
//  const json j =
//      "{\"_CG\":"
//      "  {\n"
//      "    \"edges\": [],\n"
//      "    \"nodes\": [\n"
//      "        [\n"
//      "            11868120863286193964,\n"
//      "            {\n"
//      "                \"functionName\": \"main\",\n"
//      "                \"hasBody\": true,\n"
//      "                \"origin\": \"main.cpp\",\n"
//      "                \"meta\": {}\n"
//      "            }\n"
//      "        ]\n"
//      "    ]\n"
//      "  }"
//      ",\"_MetaCG\":{"
//      "   \"generator\":{\"name\":\"Test\",\"sha\":\"TestSha\",\"version\":\"0.1\"},"
//      "    \"version\":\"3.0\"}"
//      "}"_json;
//  auto& mcgm = metacg::graph::MCGManager::get();
//  mcgm.resetManager();
//
//  metacg::io::JsonSource source(j);
//  metacg::io::VersionThreeMetaCGReader reader(source);
//  mcgm.addToManagedGraphs("newCallgraph", reader.read());
//  Callgraph& graph = *mcgm.getCallgraph();
//  EXPECT_EQ(graph.size(), 1);
//  EXPECT_NE(graph.getMain(), nullptr);
//  EXPECT_EQ(graph.getMain()->getFunctionName(), "main");
//
//  EXPECT_EQ(graph.getEdges().size(), 0);
//  EXPECT_EQ(graph.getMain()->getMetaDataContainer().size(), 0);
//}
//
//TEST(V3MCGReaderTest, NodesAndSingleEdge) {
//  const json j =
//      "{\"_CG\":"
//      "{\n"
//      "    \"edges\": [\n"
//      "        [\n"
//      "            [\n"
//      "                6996584602980610887,\n"
//      "                14321258476710963540\n"
//      "            ],\n"
//      "            []\n"
//      "        ]\n"
//      "    ],\n"
//      "    \"nodes\": [\n"
//      "        [\n"
//      "            14321258476710963540,\n"
//      "            {\n"
//      "                \"functionName\": \"Function3\",\n"
//      "                \"hasBody\": true,\n"
//      "                \"origin\": \"function3.cpp\",\n"
//      "                \"meta\": {}\n"
//      "            }\n"
//      "        ],\n"
//      "        [\n"
//      "            298926739030946466,\n"
//      "            {\n"
//      "                \"functionName\": \"Function2\",\n"
//      "                \"hasBody\": false,\n"
//      "                \"origin\": \"function1and2.cpp\",\n"
//      "                \"meta\": {}\n"
//      "            }\n"
//      "        ],\n"
//      "        [\n"
//      "            6996584602980610887,\n"
//      "            {\n"
//      "                \"functionName\": \"Function1\",\n"
//      "                \"hasBody\": true,\n"
//      "                \"origin\": \"function1and2.cpp\",\n"
//      "                \"meta\": {}\n"
//      "            }\n"
//      "        ]\n"
//      "    ]\n"
//      "  }"
//      ",\"_MetaCG\":{"
//      "   \"generator\":{\"name\":\"Test\",\"sha\":\"TestSha\",\"version\":\"0.1\"},"
//      "    \"version\":\"3.0\"}"
//      "}"_json;
//
//  auto& mcgm = metacg::graph::MCGManager::get();
//  mcgm.resetManager();
//  metacg::io::JsonSource source(j);
//  metacg::io::VersionThreeMetaCGReader reader(source);
//  mcgm.addToManagedGraphs("newCallgraph", reader.read());
//
//  const Callgraph& graph = *mcgm.getCallgraph();
//  EXPECT_EQ(graph.size(), 3);
//
//  metacg::CgNode* function1 = graph.getNode("Function1");
//  ASSERT_NE(function1, nullptr);
//  EXPECT_EQ(function1->getFunctionName(), "Function1");
//  EXPECT_EQ(function1->getHasBody(), true);
//  EXPECT_EQ(function1->getOrigin(), "function1and2.cpp");
//  EXPECT_EQ(function1->getId(), std::hash<std::string>()(function1->getFunctionName() + function1->getOrigin()));
//
//  metacg::CgNode* function2 = graph.getNode("Function2");
//  ASSERT_NE(function2, nullptr);
//  EXPECT_EQ(function2->getFunctionName(), "Function2");
//  EXPECT_EQ(function2->getHasBody(), false);
//  EXPECT_EQ(function2->getOrigin(), "function1and2.cpp");
//  EXPECT_EQ(function2->getId(), std::hash<std::string>()(function2->getFunctionName() + function2->getOrigin()));
//
//  EXPECT_EQ(function1->getOrigin(), function2->getOrigin());
//
//  metacg::CgNode* function3 = graph.getNode("Function3");
//  ASSERT_NE(function3, nullptr);
//  EXPECT_EQ(function3->getFunctionName(), "Function3");
//  EXPECT_EQ(function3->getHasBody(), true);
//  EXPECT_EQ(function3->getOrigin(), "function3.cpp");
//  EXPECT_EQ(function3->getId(), std::hash<std::string>()(function3->getFunctionName() + function3->getOrigin()));
//
//  EXPECT_FALSE(graph.existEdgeFromTo("Function1", "Function1"));
//  EXPECT_FALSE(graph.existEdgeFromTo("Function1", "Function2"));
//  EXPECT_TRUE(graph.existEdgeFromTo("Function1", "Function3"));
//
//  EXPECT_FALSE(graph.existEdgeFromTo("Function2", "Function1"));
//  EXPECT_FALSE(graph.existEdgeFromTo("Function2", "Function2"));
//  EXPECT_FALSE(graph.existEdgeFromTo("Function2", "Function3"));
//
//  EXPECT_FALSE(graph.existEdgeFromTo("Function3", "Function1"));
//  EXPECT_FALSE(graph.existEdgeFromTo("Function3", "Function2"));
//  EXPECT_FALSE(graph.existEdgeFromTo("Function3", "Function3"));
//
//  EXPECT_EQ(function1->getMetaDataContainer().size(), 0);
//}
//
//TEST(V3MCGReaderTest, NodeMetaData) {
//  const json j =
//      "{\"_CG\":"
//      "{\n"
//      "    \"edges\": [],\n"
//      "    \"nodes\": [\n"
//      "        [\n"
//      "            11868120863286193964,\n"
//      "            {\n"
//      "                \"functionName\": \"main\",\n"
//      "                \"hasBody\": true,\n"
//      "                \"isVirtual\": false,\n"
//      "                \"meta\": {\n"
//      "                    \"TestMetaDataV3\": {\n"
//      "                        \"stored_double\": 13.37,\n"
//      "                        \"stored_int\": 42,\n"
//      "                        \"stored_string\": \"Test\"\n"
//      "                    }"
//      "                }\n"
//      "            }\n"
//      "        ]\n"
//      "    ]\n"
//      "  }"
//      ",\"_MetaCG\":{"
//      "   \"generator\":{\"name\":\"Test\",\"sha\":\"TestSha\",\"version\":\"0.1\"},"
//      "    \"version\":\"3.0\"}"
//      "}"_json;
//
//  auto& mcgm = metacg::graph::MCGManager::get();
//  mcgm.resetManager();
//  metacg::io::JsonSource source(j);
//  metacg::io::VersionThreeMetaCGReader reader(source);
//  mcgm.addToManagedGraphs("newCallgraph", reader.read());
//  const Callgraph& graph = *mcgm.getCallgraph();
//
//  EXPECT_EQ(graph.getNode("main")->getMetaDataContainer().size(), 1);
//  EXPECT_NE(graph.getNode("main")->get<TestMetaDataV3>(), nullptr);
//}

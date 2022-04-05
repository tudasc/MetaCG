/**
 * File: MCGWriterTest.cpp
 * License: Part of the MetaCG project. Licensed under BSD 3 clause license. See LICENSE.txt file at
 * https://github.com/tudasc/metacg/LICENSE.txt
 */
#include "gtest/gtest.h"

#include "../../../pgis/test/unit/LoggerUtil.h"
#include "MCGManager.h"
#include "MCGWriter.h"
#include "MetaDataHandler.h"
#include "loadImbalance/LIRetriever.h"
#include "nlohmann/json.hpp"

using namespace metacg;
using json = nlohmann::json;

/**
 * MetaDataHandler used for testing
 */
struct TestHandler : public metacg::io::retriever::MetaDataHandler {
  int i{0};
  const std::string toolName() const override { return "TestMetaHandler"; }
  void read([[maybe_unused]] const json &j, const std::string &functionName) override { i++; }
  bool handles(const CgNodePtr n) const override { return false; }
  json value(const CgNodePtr n) const {
    json j;
    j = i;
    return j;
  }
};

namespace JsonFieldNames {
static const std::string cg{"_CG"};
static const std::string md{"meta"};
static const std::string bpd{"baseProfileData"};
static const std::string pod{"numStatements"};
static const std::string lid{"LIData"};
}  // namespace JsonFieldNames

TEST(MCGWriterTest, EmptyGraph) {
  loggerutil::getLogger();

  auto &mcgm = metacg::graph::MCGManager::get();
  mcgm.resetActiveGraph();
  mcgm.addMetaHandler<metacg::io::retriever::BaseProfileDataHandler>();
  mcgm.addMetaHandler<metacg::io::retriever::PiraOneDataRetriever>();
  mcgm.addMetaHandler<LoadImbalance::LoadImbalanceMetaDataHandler>();

  metacg::io::JsonSink jsSink;
  metacg::io::MCGWriter mcgw(mcgm);
  mcgw.write(jsSink);

  auto j = jsSink.getJson();
  ASSERT_TRUE(j[JsonFieldNames::cg].is_null());
}

TEST(MCGWriterTest, OneNodeGraphNoHandlerAttached) {
  loggerutil::getLogger();

  auto &mcgm = metacg::graph::MCGManager::get();
  mcgm.resetActiveGraph();
  auto mainNode = mcgm.findOrCreateNode("main");

  metacg::io::JsonSink jsSink;
  metacg::io::MCGWriter mcgw(mcgm);
  mcgw.write(jsSink);

  const auto js = jsSink.getJson();
  ASSERT_FALSE(js[JsonFieldNames::cg].is_null());

  auto bpData = js[JsonFieldNames::cg]["main"][JsonFieldNames::md];
  for (auto it = bpData.begin(); it != bpData.end(); ++it) {
    ASSERT_NE(it.key(), JsonFieldNames::bpd);
    ASSERT_NE(it.key(), "numStatements");
    ASSERT_NE(it.key(), "LIData");
  }
}

TEST(MCGWriterTest, OneNodeGraph) {
  loggerutil::getLogger();

  auto &mcgm = metacg::graph::MCGManager::get();
  mcgm.resetActiveGraph();
  auto mainNode = mcgm.findOrCreateNode("main");
  mcgm.addMetaHandler<metacg::io::retriever::BaseProfileDataHandler>();
  mcgm.addMetaHandler<metacg::io::retriever::PiraOneDataRetriever>();
  mcgm.addMetaHandler<LoadImbalance::LoadImbalanceMetaDataHandler>();

  // How to use the MCGWriter
  metacg::io::JsonSink jsSink;
  metacg::io::MCGWriter mcgw(mcgm);
  mcgw.write(jsSink);

  // Test that the fields are present in the generated json
  const auto js = jsSink.getJson();
  ASSERT_FALSE(js[JsonFieldNames::cg].is_null());

  // BaseProfileData
  auto bpData = js[JsonFieldNames::cg]["main"][JsonFieldNames::md][JsonFieldNames::bpd];
  ASSERT_FALSE(bpData["numCalls"].is_null());
  ASSERT_EQ(bpData["numCalls"].get<unsigned long long>(), 0);
  ASSERT_EQ(bpData["timeInSeconds"].get<double>(), .0);
  ASSERT_EQ(bpData["inclusiveRtInSeconds"].get<double>(), .0);

  // PiraOneData, Fixme: the toolname of PIRA one data is old
  auto poData = js[JsonFieldNames::cg]["main"][JsonFieldNames::md][JsonFieldNames::pod];
  ASSERT_FALSE(poData["numStatements"].is_null());
  ASSERT_EQ(poData["numStatements"].get<int>(), 0);

  // LoadImbalanceDetection data
  auto liData = js[JsonFieldNames::cg]["main"][JsonFieldNames::md][JsonFieldNames::lid];
  ASSERT_FALSE(liData.is_null());
  ASSERT_FALSE(liData["irrelevant"].is_null());
  ASSERT_FALSE(liData["visited"].is_null());
  ASSERT_EQ(liData["irrelevant"].get<bool>(), false);
  ASSERT_EQ(liData["visited"].get<bool>(), false);
}

TEST(MCGWriterTest, OneNodeGraphWithData) {
  loggerutil::getLogger();

  auto &mcgm = metacg::graph::MCGManager::get();
  mcgm.resetActiveGraph();
  auto mainNode = mcgm.findOrCreateNode("main");
  mcgm.addMetaHandler<metacg::io::retriever::BaseProfileDataHandler>();
  mcgm.addMetaHandler<metacg::io::retriever::PiraOneDataRetriever>();
  mcgm.addMetaHandler<LoadImbalance::LoadImbalanceMetaDataHandler>();

  {
    auto bpd = mainNode->get<pira::BaseProfileData>();
    bpd->setNumberOfCalls(42);
    bpd->setRuntimeInSeconds(21);
    bpd->setInclusiveRuntimeInSeconds(.666);
    auto pod = mainNode->get<pira::PiraOneData>();
    pod->setNumberOfStatements(64);
  }

  // How to use the MCGWriter
  metacg::io::JsonSink jsSink;
  metacg::io::MCGWriter mcgw(mcgm);
  mcgw.write(jsSink);

  // Test that the fields are present in the generated json
  const auto js = jsSink.getJson();
  ASSERT_FALSE(js[JsonFieldNames::cg].is_null());

  // BaseProfileData
  auto bpData = js[JsonFieldNames::cg]["main"][JsonFieldNames::md][JsonFieldNames::bpd];
  ASSERT_FALSE(bpData["numCalls"].is_null());
  ASSERT_EQ(bpData["numCalls"].get<unsigned long long>(), 42);
  ASSERT_EQ(bpData["timeInSeconds"].get<double>(), 21);
  ASSERT_EQ(bpData["inclusiveRtInSeconds"].get<double>(), .666);

  // PiraOneData, Fixme: the toolname of PIRA one data is old
  auto poData = js[JsonFieldNames::cg]["main"][JsonFieldNames::md][JsonFieldNames::pod];
  ASSERT_FALSE(poData["numStatements"].is_null());
  ASSERT_EQ(poData["numStatements"].get<int>(), 64);

  // LoadImbalanceDetection data
  auto liData = js[JsonFieldNames::cg]["main"][JsonFieldNames::md][JsonFieldNames::lid];
  ASSERT_FALSE(liData.is_null());
  ASSERT_FALSE(liData["irrelevant"].is_null());
  ASSERT_FALSE(liData["visited"].is_null());
  ASSERT_EQ(liData["irrelevant"].get<bool>(), false);
  ASSERT_EQ(liData["visited"].get<bool>(), false);
}

TEST(MCGWriterTest, OneNodeGraphNoDataForHandler) {
  loggerutil::getLogger();

  auto &mcgm = metacg::graph::MCGManager::get();
  mcgm.resetActiveGraph();
  auto mainNode = mcgm.findOrCreateNode("main");
  mcgm.addMetaHandler<metacg::io::retriever::GlobalLoopDepthHandler>();

  // How to use the MCGWriter
  metacg::io::JsonSink jsSink;
  metacg::io::MCGWriter mcgw(mcgm);
  mcgw.write(jsSink);

  // Test that the fields are present in the generated json
  const auto js = jsSink.getJson();
  ASSERT_FALSE(js[JsonFieldNames::cg].is_null());

  // Global Loop Depth not attached, no meta data
  auto metaData = js[JsonFieldNames::cg]["main"][JsonFieldNames::md];
  ASSERT_TRUE(metaData.is_null());
}
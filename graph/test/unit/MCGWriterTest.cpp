/**
 * File: MCGWriterTest.cpp
 * License: Part of the MetaCG project. Licensed under BSD 3 clause license. See LICENSE.txt file at
 * https://github.com/tudasc/metacg/LICENSE.txt
 */
#include "gtest/gtest.h"

#include "LoggerUtil.h"
#include "MCGManager.h"
#include "MCGWriter.h"
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
  bool handles(const CgNode *const n) const override { return false; }
  json value(const CgNode *const n) const override {
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
  metacg::loggerutil::getLogger();

  auto &mcgm = metacg::graph::MCGManager::get();
  mcgm.resetManager();
  mcgm.addToManagedGraphs("emptyGraph", std::make_unique<Callgraph>());
  mcgm.addMetaHandler<TestHandler>();

  metacg::io::JsonSink jsSink;
  metacg::io::MCGWriter mcgw(mcgm);
  mcgw.write(jsSink);

  auto j = jsSink.getJson();
  ASSERT_TRUE(j[JsonFieldNames::cg].is_null());
}

TEST(MCGWriterTest, OneNodeGraphNoHandlerAttached) {
  metacg::loggerutil::getLogger();

  auto &mcgm = metacg::graph::MCGManager::get();
  mcgm.resetActiveGraph();
  auto mainNode = mcgm.getCallgraph()->getOrInsertNode("main");

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

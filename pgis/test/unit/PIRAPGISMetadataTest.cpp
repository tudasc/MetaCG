/**
 * File: LegacyMCGReaderTest.cpp
 * License: Part of the metacg project. Licensed under BSD 3 clause license. See LICENSE.txt file at
 * https://github.com/tudasc/metacg/LICENSE.txt
 */

#include "MetaData/CgNodeMetaData.h"
#include "loadImbalance/LIMetaData.h"
#include "gtest/gtest.h"

#include "MetaData/PGISMetaData.h"
#include "io/IdMapping.h"
#include "nlohmann/json.hpp"

using namespace metacg;
using json = nlohmann::json;

struct NoStrMapping : public metacg::StrToNodeMapping {
  CgNode* getNodeFromStr(const std::string&) override {
    return nullptr;
  }
};

struct NoNodeMapping : public metacg::NodeToStrMapping {
  std::string getStrFromNode(NodeId id) override {
    return "";
  }
};

TEST(PIRAPGISMetadataTest, BaseProfileData) {
  // Generate "Real" Object
  const auto& origMetadata = new pira::BaseProfileData();

  // Export "Real" Object
  NoNodeMapping nodeMapping;
  const auto& j = origMetadata->toJson(nodeMapping);
  // Compare all exported values
  EXPECT_TRUE(j.contains("numCalls"));
  EXPECT_EQ(j.at("numCalls"), origMetadata->getNumberOfCalls());
  EXPECT_TRUE(j.contains("timeInSeconds"));
  EXPECT_EQ(j.at("timeInSeconds"), origMetadata->getRuntimeInSeconds());
  EXPECT_TRUE(j.contains("inclusiveRtInSeconds"));
  EXPECT_EQ(j.at("inclusiveRtInSeconds"), origMetadata->getInclusiveRuntimeInSeconds());

  // Import Json into new Object
  NoStrMapping strMapping;
  const auto& jsonMetadata = new pira::BaseProfileData(j, strMapping);

  // Compare all imported values
  EXPECT_EQ(jsonMetadata->getNumberOfCalls(), origMetadata->getNumberOfCalls());

  EXPECT_EQ(jsonMetadata->getRuntimeInSeconds(), origMetadata->getRuntimeInSeconds());

  EXPECT_EQ(jsonMetadata->getInclusiveRuntimeInSeconds(), origMetadata->getInclusiveRuntimeInSeconds());
  delete jsonMetadata;
  delete origMetadata;
}

TEST(PIRAPGISMetadataTest, PiraOneData) {
  // Generate "Real" Object
  const auto& origMetadata = new pira::PiraOneData();
  // Export "Real" Object
  NoNodeMapping nodeMapping;
  const auto& j = origMetadata->toJson(nodeMapping);

  // Compare all exported values
  EXPECT_EQ(j.get<long long int>(), origMetadata->getNumberOfStatements());

  // Import Json Object
  // Note: due to historical reasons, PiraOne Data is not a json object, but only a single value
  NoStrMapping strMapping;
  const auto& jsonMetadata = new pira::PiraOneData(j, strMapping);

  // Compare all imported values
  EXPECT_EQ(jsonMetadata->getNumberOfStatements(), origMetadata->getNumberOfStatements());
  delete origMetadata;
  delete jsonMetadata;
}

TEST(PIRAPGISMetadataTest, PiraTwoData) {
  // Generate "Real" Object
  const auto& origMetadata = new pira::PiraTwoData();

  // Export "Real" Object
  NoNodeMapping nodeMapping;
  const auto& j = origMetadata->toJson(nodeMapping);

  // Compare all exported values
  EXPECT_TRUE(j.is_null());

  // Import Json Object
  NoStrMapping strMapping;
  const auto& jsonMetadata = new pira::PiraTwoData(j, strMapping);

  // Compare all exported / imported values
  delete origMetadata;
  delete jsonMetadata;
}

TEST(PIRAPGISMetadataTest, FilePropertiesMetaData) {
  // Generate "Real" Object
  const auto& origMetadata = new pira::FilePropertiesMetaData();

  // Export "Real" Object
  NoNodeMapping nodeMapping;
  const auto& j = origMetadata->toJson(nodeMapping);
  // Compare all exported values
  EXPECT_TRUE(j.contains("origin"));
  EXPECT_EQ(j.at("origin"), origMetadata->origin);
  EXPECT_TRUE(j.contains("systemInclude"));
  EXPECT_EQ(j.at("systemInclude"), origMetadata->fromSystemInclude);

  // Import Json into new Object
  NoStrMapping strMapping;
  const auto& jsonMetadata = new pira::FilePropertiesMetaData(j, strMapping);

  // Compare all imported values
  EXPECT_EQ(jsonMetadata->origin, origMetadata->origin);

  EXPECT_EQ(jsonMetadata->fromSystemInclude, origMetadata->fromSystemInclude);
  delete origMetadata;
  delete jsonMetadata;
}

TEST(PIRAPGISMetadataTest, CodeStatisticsMD) {
  // Generate "Real" Object
  const auto& origMetadata = new CodeStatisticsMD();

  // Export "Real" Object
  NoNodeMapping nodeMapping;
  const auto& j = origMetadata->toJson(nodeMapping);
  // Compare all exported values
  EXPECT_TRUE(j.contains("numVars"));
  EXPECT_EQ(j.at("numVars"), origMetadata->numVars);

  // Import Json into new Object
  NoStrMapping strMapping;
  const auto& jsonMetadata = new CodeStatisticsMD(j, strMapping);

  // Compare all imported values
  EXPECT_EQ(jsonMetadata->numVars, origMetadata->numVars);
  delete origMetadata;
  delete jsonMetadata;
}

TEST(PIRAPGISMetadataTest, NumConditionalBranchMetaData) {
  // Generate "Real" Object
  const auto& origMetadata = new NumConditionalBranchMD();

  // Export "Real" Object
  NoNodeMapping nodeMapping;
  const auto& j = origMetadata->toJson(nodeMapping);
  // Compare all exported values
  // Note: due to historical reasons, NumConditionalBranchMD is not a json object, but only a single value
  EXPECT_EQ(j.get<int>(), origMetadata->numConditionalBranches);

  // Import Json into new Object
  NoStrMapping strMapping;
  const auto& jsonMetadata = new NumConditionalBranchMD(j, strMapping);

  // Compare all imported values
  EXPECT_EQ(jsonMetadata->numConditionalBranches, origMetadata->numConditionalBranches);
  delete origMetadata;
  delete jsonMetadata;
}

TEST(PIRAPGISMetadataTest, NumOperationsMetaData) {
  // Generate "Real" Object
  const auto& origMetadata = new NumOperationsMD();
  // Export "Real" Object
  NoNodeMapping nodeMapping;
  const auto& j = origMetadata->toJson(nodeMapping);
  // Compare all exported values
  EXPECT_TRUE(j.contains("numberOfIntOps"));
  EXPECT_EQ(j.at("numberOfIntOps"), origMetadata->numberOfIntOps);
  EXPECT_TRUE(j.contains("numberOfFloatOps"));
  EXPECT_EQ(j.at("numberOfFloatOps"), origMetadata->numberOfFloatOps);
  EXPECT_TRUE(j.contains("numberOfControlFlowOps"));
  EXPECT_EQ(j.at("numberOfControlFlowOps"), origMetadata->numberOfControlFlowOps);
  EXPECT_TRUE(j.contains("numberOfMemoryAccesses"));
  EXPECT_EQ(j.at("numberOfMemoryAccesses"), origMetadata->numberOfMemoryAccesses);

  // Import Json into new Object
  NoStrMapping strMapping;
  const auto& jsonMetadata = new NumOperationsMD(j, strMapping);

  // Compare all imported values
  EXPECT_EQ(jsonMetadata->numberOfIntOps, origMetadata->numberOfIntOps);
  EXPECT_EQ(jsonMetadata->numberOfFloatOps, origMetadata->numberOfFloatOps);
  EXPECT_EQ(jsonMetadata->numberOfControlFlowOps, origMetadata->numberOfControlFlowOps);
  EXPECT_EQ(jsonMetadata->numberOfMemoryAccesses, origMetadata->numberOfMemoryAccesses);
  delete origMetadata;
  delete jsonMetadata;
}

TEST(PIRAPGISMetadataTest, LoopDepthMD) {
  // Generate "Real" Object
  const auto& origMetadata = new LoopDepthMD();

  // Export "Real" Object
  NoNodeMapping nodeMapping;
  const auto& j = origMetadata->toJson(nodeMapping);
  // Compare all exported values
  // Note: due to historical reasons, LoopDepthMD is not a json object, but only a single value
  EXPECT_EQ(j.get<int>(), origMetadata->loopDepth);

  // Import Json into new Object
  NoStrMapping strMapping;
  const auto& jsonMetadata = new LoopDepthMD(j, strMapping);

  // Compare all imported values
  EXPECT_EQ(jsonMetadata->loopDepth, origMetadata->loopDepth);
  delete origMetadata;
  delete jsonMetadata;
}

TEST(PIRAPGISMetadataTest, GlobalLoopDepthMD) {
  // Generate "Real" Object
  const auto& origMetadata = new GlobalLoopDepthMD();

  // Export "Real" Object
  NoNodeMapping nodeMapping;
  const auto& j = origMetadata->toJson(nodeMapping);
  // Compare all exported values
  // Note: due to historical reasons, GlobalLoopDepthMD is not a json object, but only a single value
  EXPECT_EQ(j.get<int>(), origMetadata->globalLoopDepth);

  // Import Json into new Object

  NoStrMapping strMapping;
  const auto& jsonMetadata = new GlobalLoopDepthMD(j, strMapping);

  // Compare all imported values
  EXPECT_EQ(jsonMetadata->globalLoopDepth, origMetadata->globalLoopDepth);
  delete origMetadata;
  delete jsonMetadata;
}

TEST(PIRAPGISMetadataTest, InstrumentationMetaData) {
  // Generate "Real" Object
  const auto& origMetadata = new pgis::InstrumentationMetaData();

  // Export "Real" Object
  NoNodeMapping nodeMapping;
  const auto& j = origMetadata->toJson(nodeMapping);
  // Compare all exported values
  EXPECT_TRUE(j.is_null());

  // Import Json into new Object
  NoStrMapping strMapping;
  const auto& jsonMetadata = new pgis::InstrumentationMetaData(j, strMapping);

  // Compare all imported values
  delete origMetadata;
  delete jsonMetadata;
}

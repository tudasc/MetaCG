/**
 * File: LegacyMCGReaderTest.cpp
 * License: Part of the metacg project. Licensed under BSD 3 clause license. See LICENSE.txt file at
 * https://github.com/tudasc/metacg/LICENSE.txt
 */

#include "MetaData/CgNodeMetaData.h"
#include "loadImbalance/LIMetaData.h"
#include "gtest/gtest.h"

#include "MetaData/PGISMetaData.h"
#include "nlohmann/json.hpp"

using namespace metacg;
using json = nlohmann::json;

TEST(PIRAPGISMetadataTest, BaseProfileData) {
  // Generate "Real" Object
  const auto &origMetadata = new pira::BaseProfileData();

  // Export "Real" Object
  const auto &j = origMetadata->to_json();
  // Compare all exported values
  EXPECT_TRUE(j.contains("numCalls"));
  EXPECT_EQ(j.at("numCalls"), origMetadata->getNumberOfCalls());
  EXPECT_TRUE(j.contains("timeInSeconds"));
  EXPECT_EQ(j.at("timeInSeconds"), origMetadata->getRuntimeInSeconds());
  EXPECT_TRUE(j.contains("inclusiveRtInSeconds"));
  EXPECT_EQ(j.at("inclusiveRtInSeconds"), origMetadata->getInclusiveRuntimeInSeconds());

  // Import Json into new Object
  const auto &jsonMetadata = new pira::BaseProfileData(j);

  // Compare all imported values
  EXPECT_EQ(jsonMetadata->getNumberOfCalls(), origMetadata->getNumberOfCalls());

  EXPECT_EQ(jsonMetadata->getRuntimeInSeconds(), origMetadata->getRuntimeInSeconds());

  EXPECT_EQ(jsonMetadata->getInclusiveRuntimeInSeconds(), origMetadata->getInclusiveRuntimeInSeconds());
}

TEST(PIRAPGISMetadataTest, PiraOneData) {
  // Generate "Real" Object
  const auto &origMetadata = new pira::PiraOneData();
  // Export "Real" Object
  const auto &j = origMetadata->to_json();

  // Compare all exported values
  EXPECT_TRUE(j.contains("numStatements"));
  EXPECT_EQ(j.at("numStatements"), origMetadata->getNumberOfStatements());

  // Import Json Object
  // Note: due to historical reasons, PiraOne Data is not a json object, but only a single value
  const auto &jsonMetadata = new pira::PiraOneData(j.at("numStatements"));

  // Compare all imported values
  EXPECT_EQ(jsonMetadata->getNumberOfStatements(), origMetadata->getNumberOfStatements());
}

TEST(PIRAPGISMetadataTest, PiraTwoData) {
  // Generate "Real" Object
  const auto &origMetadata = new pira::PiraTwoData();

  // Export "Real" Object
  const auto &j = origMetadata->to_json();

  // Compare all exported values
  EXPECT_TRUE(j.is_null());

  // Import Json Object
  const auto &jsonMetadata = new pira::PiraTwoData(j);

  // Compare all exported / imported values
}

TEST(PIRAPGISMetadataTest, FilePropertiesMetaData) {
  // Generate "Real" Object
  const auto &origMetadata = new pira::FilePropertiesMetaData();

  // Export "Real" Object
  const auto &j = origMetadata->to_json();
  // Compare all exported values
  EXPECT_TRUE(j.contains("origin"));
  EXPECT_EQ(j.at("origin"), origMetadata->origin);
  EXPECT_TRUE(j.contains("systemInclude"));
  EXPECT_EQ(j.at("systemInclude"), origMetadata->fromSystemInclude);

  // Import Json into new Object
  const auto &jsonMetadata = new pira::FilePropertiesMetaData(j);

  // Compare all imported values
  EXPECT_EQ(jsonMetadata->origin, origMetadata->origin);

  EXPECT_EQ(jsonMetadata->fromSystemInclude, origMetadata->fromSystemInclude);
}

TEST(PIRAPGISMetadataTest, CodeStatisticsMetaData) {
  // Generate "Real" Object
  const auto &origMetadata = new pira::CodeStatisticsMetaData();

  // Export "Real" Object
  const auto &j = origMetadata->to_json();
  // Compare all exported values
  EXPECT_TRUE(j.contains("numVars"));
  EXPECT_EQ(j.at("numVars"), origMetadata->numVars);

  // Import Json into new Object
  const auto &jsonMetadata = new pira::CodeStatisticsMetaData(j);

  // Compare all imported values
  EXPECT_EQ(jsonMetadata->numVars, origMetadata->numVars);
}

TEST(PIRAPGISMetadataTest, NumConditionalBranchMetaData) {
  // Generate "Real" Object
  const auto &origMetadata = new pira::NumConditionalBranchMetaData();

  // Export "Real" Object
  const auto &j = origMetadata->to_json();
  // Compare all exported values
  // Note: due to historical reasons, NumConditionalBranchMetaData is not a json object, but only a single value
  EXPECT_EQ(j.get<int>(), origMetadata->numConditionalBranches);

  // Import Json into new Object
  const auto &jsonMetadata = new pira::NumConditionalBranchMetaData(j);

  // Compare all imported values
  EXPECT_EQ(jsonMetadata->numConditionalBranches, origMetadata->numConditionalBranches);
}

TEST(PIRAPGISMetadataTest, NumOperationsMetaData) {
  // Generate "Real" Object
  const auto &origMetadata = new pira::NumOperationsMetaData();

  // Export "Real" Object
  const auto &j = origMetadata->to_json();
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
  const auto &jsonMetadata = new pira::NumOperationsMetaData(j);

  // Compare all imported values
  EXPECT_EQ(jsonMetadata->numberOfIntOps, origMetadata->numberOfIntOps);
  EXPECT_EQ(jsonMetadata->numberOfFloatOps, origMetadata->numberOfFloatOps);
  EXPECT_EQ(jsonMetadata->numberOfControlFlowOps, origMetadata->numberOfControlFlowOps);
  EXPECT_EQ(jsonMetadata->numberOfMemoryAccesses, origMetadata->numberOfMemoryAccesses);
}

TEST(PIRAPGISMetadataTest, LoopDepthMetaData) {
  // Generate "Real" Object
  const auto &origMetadata = new pira::LoopDepthMetaData();

  // Export "Real" Object
  const auto &j = origMetadata->to_json();
  // Compare all exported values
  // Note: due to historical reasons, LoopDepthMetaData is not a json object, but only a single value
  EXPECT_EQ(j.get<int>(), origMetadata->loopDepth);

  // Import Json into new Object
  const auto &jsonMetadata = new pira::LoopDepthMetaData(j);

  // Compare all imported values
  EXPECT_EQ(jsonMetadata->loopDepth, origMetadata->loopDepth);
}

TEST(PIRAPGISMetadataTest, GlobalLoopDepthMetaData) {
  // Generate "Real" Object
  const auto &origMetadata = new pira::GlobalLoopDepthMetaData();

  // Export "Real" Object
  const auto &j = origMetadata->to_json();
  // Compare all exported values
  // Note: due to historical reasons, GlobalLoopDepthMetaData is not a json object, but only a single value
  EXPECT_EQ(j.get<int>(), origMetadata->globalLoopDepth);

  // Import Json into new Object

  const auto &jsonMetadata = new pira::GlobalLoopDepthMetaData(j);

  // Compare all imported values
  EXPECT_EQ(jsonMetadata->globalLoopDepth, origMetadata->globalLoopDepth);
}

TEST(PIRAPGISMetadataTest, InstrumentationMetaData) {
  // Generate "Real" Object
  const auto &origMetadata = new pgis::InstrumentationMetaData();

  // Export "Real" Object
  const auto &j = origMetadata->to_json();
  // Compare all exported values
  EXPECT_TRUE(j.is_null());

  // Import Json into new Object
  const auto &jsonMetadata = new pgis::InstrumentationMetaData(j);

  // Compare all imported values
}

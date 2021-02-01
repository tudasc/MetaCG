#include "../LoggerUtil.h"
#include "nlohmann/json.hpp"
#include "gtest/gtest.h"
#include <loadImbalance/Config.h>

using namespace nlohmann;
using namespace LoadImbalance;

class LIConfigTest : public ::testing::Test {
 protected:
  void SetUp() override { loggerutil::getLogger(); }
};

TEST_F(LIConfigTest, SimpleTest) {
  json j = {
      {"metricType", "Efficiency"},
      {"imbalanceThreshold", 1.0},
      {"relevanceThreshold", 1.0},
      {"contextStrategy", "AllPathsToMain"},
      {"contextStepCount", 5},
      {"childRelevanceStrategy", "ConstantThreshold"},
      {"childConstantThreshold", 5},
      {"childFraction", 0.5},
  };

  LoadImbalance::Config c = j;

  ASSERT_EQ(c.metricType, MetricType::Efficiency);
  ASSERT_EQ(c.contextStrategy, ContextStrategy::AllPathsToMain);
  ASSERT_EQ(c.childRelevanceStrategy, ChildRelevanceStrategy::ConstantThreshold);
  ASSERT_EQ(c.imbalanceThreshold, 1.0);
  ASSERT_EQ(c.relevanceThreshold, 1.0);
  ASSERT_EQ(c.contextStepCount, 5);
  ASSERT_EQ(c.childConstantThreshold, 5);
  ASSERT_EQ(c.childFraction, 0.5);
}

TEST_F(LIConfigTest, Empty) {
  json j = {};

  ASSERT_THROW(LoadImbalance::Config c = j, json::exception);
}

TEST_F(LIConfigTest, Missing) {
  json j = {
      {"metricType", "Efficiency"},
      {"imbalanceThreshold", 1.0},
      // {"relevanceThreshold", 1.0},
      {"contextStrategy", "AllPathsToMain"},
      {"contextStepCount", 5},
      {"childRelevanceStrategy", "ConstantThreshold"},
      {"childConstantThreshold", 5},
      {"childFraction", 0.5},
  };

  ASSERT_THROW(LoadImbalance::Config c = j, json::exception);
}
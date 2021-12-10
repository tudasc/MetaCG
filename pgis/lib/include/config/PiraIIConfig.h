/**
 * File: PiraIIConfig.h
 * License: Part of the MetaCG project. Licensed under BSD 3 clause license. See LICENSE.txt file at
 * https://github.com/tudasc/metacg/LICENSE.txt
 */

#ifndef PGIS_PIRAIICONFIG_H
#define PGIS_PIRAIICONFIG_H

#include "nlohmann/json.hpp"

namespace pgis::config {
/**
 * Strategies to aggregate multiple models for a single function into one
 */
enum class ModelAggregationStrategy {
  // use the first model exclusively
  FirstModel,

  // use the sum of all models
  Sum,

  // calculate the mean of the models (sum devided by the number of models)
  Average,

  // calculate the maximum function from the availabel models
  Maximum
};

struct PiraIIConfig {
  double extrapolationThreshold;
  int statementThreshold;
  ModelAggregationStrategy modelAggregationStrategy;
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(PiraIIConfig, extrapolationThreshold, statementThreshold, modelAggregationStrategy);

NLOHMANN_JSON_SERIALIZE_ENUM(ModelAggregationStrategy, {
                                                           {ModelAggregationStrategy::FirstModel, "FirstModel"},
                                                           {ModelAggregationStrategy::Sum, "Sum"},
                                                           {ModelAggregationStrategy::Average, "Average"},
                                                           {ModelAggregationStrategy::Maximum, "Maximum"},
                                                       })
}  // namespace pgis::config

#endif  // PGIS_PIRAIICONFIG_H

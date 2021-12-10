/**
 * File: LIConfig.h
 * License: Part of the MetaCG project. Licensed under BSD 3 clause license. See LICENSE.txt file at
 * https://github.com/tudasc/metacg/LICENSE.txt
 */

#ifndef LI_CONFIG_H
#define LI_CONFIG_H

#include "CgNode.h"
#include "nlohmann/json.hpp"

namespace LoadImbalance {

/**
 * Available metric to quantify load imbalance for a node
 */
enum class MetricType { Efficiency, VariationCoeff, ImbalancePercentage };

/**
 * Available strategies to process the context of nodes, which have been detected as load imbalanced
 */
enum class ContextStrategy { AllPathsToMain, MajorPathsToMain, MajorParentSteps, None, FindSynchronizationPoints };

/**
 * Available strategies for "iterative descent" in load imbalance detection
 */
enum class ChildRelevanceStrategy { ConstantThreshold, RelativeToMain, RelativeToParent, All };

/**
 * Set of configuration options for load imbalance detection.
 *
 * Is read from load imbalance config file
 */
struct LIConfig {
  MetricType metricType;      // which metric to use to quantify load imbalance
  double imbalanceThreshold;  // minimal metric result for a function to be counted as imbalanced
  double relevanceThreshold;  // minimal fraction of whole-program inclusive runtime for a function to be relevant

  ContextStrategy contextStrategy;  // how to handle the context of imbalanced functions
  unsigned int contextStepCount;    // steps to perform in case of ContextStrategy::MajorParentSteps

  ChildRelevanceStrategy childRelevanceStrategy;  // how to decide which children (of a relevant function) to instrument
  pira::Statements childConstantThreshold;  // constant threshold for ChildRelevanceStrategy::ConstantThreshold (and
                                            // minimum for relative cases)
  double childFraction;  // fractional for ChildRelevanceStrategy::RelativeToMain and ::RelativeToParent
};

// Nlohmann-Json-macros for serialization
// ======================================

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(LIConfig, metricType, imbalanceThreshold, relevanceThreshold, contextStrategy,
                                   contextStepCount, childRelevanceStrategy, childConstantThreshold, childFraction)

NLOHMANN_JSON_SERIALIZE_ENUM(MetricType, {
                                             {MetricType::Efficiency, "Efficiency"},
                                             {MetricType::VariationCoeff, "VariationCoeff"},
                                             {MetricType::ImbalancePercentage, "ImbalancePercentage"},
                                         })

NLOHMANN_JSON_SERIALIZE_ENUM(ContextStrategy,
                             {{ContextStrategy::AllPathsToMain, "AllPathsToMain"},
                              {ContextStrategy::MajorPathsToMain, "MajorPathsToMain"},
                              {ContextStrategy::MajorParentSteps, "MajorParentSteps"},
                              {ContextStrategy::None, "None"},
                              {ContextStrategy::FindSynchronizationPoints, "FindSynchronizationPoints"}})

NLOHMANN_JSON_SERIALIZE_ENUM(ChildRelevanceStrategy,
                             {
                                 {ChildRelevanceStrategy::ConstantThreshold, "ConstantThreshold"},
                                 {ChildRelevanceStrategy::RelativeToMain, "RelativeToMain"},
                                 {ChildRelevanceStrategy::RelativeToParent, "RelativeToParent"},
                                 {ChildRelevanceStrategy::All, "All"},
                             })
}  // namespace LoadImbalance

#endif

/**
 * File: NodeSummary.h
 * License: Part of the MetaCG project. Licensed under BSD 3 clause license. See LICENSE.txt file at
 * https://github.com/tudasc/metacg/LICENSE.txt
 */
#pragma once
#include <Callgraph.h>
#include <string>

/**
 * Summary of a node in a call graph for comparison purposes.
 */
struct NodeSummary {
  std::string name;
  bool hasBody;
  std::unordered_set<std::string> callees;  // use function names as unique identifier
  std::unordered_set<std::string>
      metadata;  // use strings instead of json to make non-order-dependant comparison easier

  std::unordered_map<std::string, std::unordered_set<std::string>> edgeMetadata;  // map callee -> metadata

  template <typename S>
  explicit NodeSummary(S&& name, bool hasBody, std::unordered_set<std::string> callees = {},
                       std::unordered_set<std::string> metadata = {},
                       std::unordered_map<std::string, std::unordered_set<std::string>> edgeMetadata = {})
      : name(std::forward<S>(name)),
        hasBody(hasBody),
        callees(std::move(callees)),
        metadata(std::move(metadata)),
        edgeMetadata(std::move(edgeMetadata)) {}
};

/**
 * Flags controlling what parts of a call graph are ignored during comparison.
 *
 * Can be compined using bitwise OR to enable multiple options.
 */
enum ComparisonMode {
  ignoreEdges = 1 << 0,
  ignoreBody = 1 << 1,
  ignoreMetadata = 1 << 2,
  ignoreEdgeMetadata = 1 << 3,
  ignoreGlobalMetadata = 1 << 4,
  none = 1 << 5
};

/**
 * Enable combining ComparisonMode flags with bitwise OR.
 */
inline ComparisonMode operator|(ComparisonMode a, ComparisonMode b) {
  return static_cast<ComparisonMode>(static_cast<int>(a) | static_cast<int>(b));
}

/**
 * Check whether a specific flag is set in a ComparisonMode bitmask.
 *
 * @param mode The combined ComparisonMode flags
 * @param flag The specific flag to check
 * @return true if the flag is set, false otherwise
 */
inline bool hasFlag(ComparisonMode mode, ComparisonMode flag) {
  return (static_cast<int>(mode) & static_cast<int>(flag)) != 0;
}

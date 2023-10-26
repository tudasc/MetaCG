/**
 * File: Utility.h
 * License: Part of the metacg project. Licensed under BSD 3 clause license. See LICENSE.txt file at
 * https://github.com/tudasc/metacg/LICENSE.txt
 */

#ifndef PGIS_UTILITY_H
#define PGIS_UTILITY_H

#include "Callgraph.h"
#include "CgNode.h"
#include "LoggerUtil.h"

#include "MetaData/PGISMetaData.h"

#include <cerrno>
#include <cstring>
#include <string>
#include <unistd.h>

namespace utils {
namespace string {

inline bool stringEndsWith(const std::string &s, const std::string &suffix) {
  return s.size() >= suffix.size() && s.compare(s.size() - suffix.size(), suffix.size(), suffix) == 0;
}

}  // namespace string

inline std::string getHostname() {
  char *cName = (char *)malloc(255 * sizeof(char));
  if (!gethostname(cName, 255)) {
    metacg::MCGLogger::instance().getErrConsole()->error("Unable to determine hostname");
  } else {
    metacg::MCGLogger::instance().getConsole()->debug("Running on host: {}", cName);
  }
  std::string name(cName);
  free(cName);
  return name;
}

/**
 * TODO This works for only a single parameter for now!
 */
template <typename C1, typename C2>
auto valTup(C1 co, C2 ct, int numReps) {
  // TODO This assert seems wrong
  // assert(co.size() == ct.size() && "Can only value-tuple evenly sized containers");
  std::vector<
      std::pair<typename C1::value_type, std::pair<std::string, typename C2::value_type::second_type::value_type>>>
      res;
  if (ct.empty()) {
    return res;
  }
  assert(ct.size() == 1 && "Current limitation, only single parameter possible");
  auto coIt = std::begin(co);
  // Compute the median per numReps from co first.
  const auto median = [&](auto startIt, auto numElems) {
    if (numElems % 2 == 0) {
      return startIt[(numElems / 2) + 1];
    }
    return startIt[numElems / 2];
  };
  auto innerC = ct[0].second;
  auto ctIt = std::begin(innerC);
  res.reserve(co.size());
  for (; coIt != co.end() && ctIt != innerC.end(); std::advance(coIt, numReps), ++ctIt) {
    res.push_back(std::make_pair(median(coIt, numReps), std::make_pair(ct[0].first, *ctIt)));
  }
  return res;
}

}  // namespace utils

inline bool isEligibleForPathInstrumentation(metacg::CgNode *node, metacg::Callgraph *graph,
                                             std::vector<metacg::CgNode *> parents = {}) {
  if (!parents.empty()) {
    return std::all_of(parents.begin(), parents.end(), [](const auto &p) { return p->getHasBody(); });
  }
  const auto &callers = graph->getCallers(node);
    return std::any_of(callers.begin(), callers.end(), [](const auto &p) { return p->getHasBody(); });
}

inline bool isLeafInstrumentationNode(metacg::CgNode *node, metacg::Callgraph *graph) {
  const auto &callees = graph->getCallees(node);
  return pgis::isAnyInstrumented(node) && std::none_of(callees.begin(), callees.end(), [&node](const auto &p) {
           return (node != p) && pgis::isAnyInstrumented(p);
         });
}

inline bool isMPIFunction(const metacg::CgNode *node) { return node->getFunctionName().rfind("MPI_") == 0; }

// Calculates the median of a container or the average of its two middle elements if it has an even number of elements.
template <typename T>
double calculateMedianAveraged(T container) {
  assert(!container.empty() && "Can not calculate median for empty container");
  const auto size = container.size();
  const auto middle = size / 2;
  std::nth_element(container.begin(), container.begin() + middle, container.end());
  if (size % 2 == 1) {
    return container[middle];
  } else {
    const auto right = container[middle];
    std::nth_element(container.begin(), container.begin() + middle - 1, container.begin() + middle);
    const auto left = container[middle - 1];
    return (left + right) / 2.0;
  }
}

#endif

/**
 * File: Utility.h
 * License: Part of the metacg project. Licensed under BSD 3 clause license. See LICENSE.txt file at
 * https://github.com/tudasc/metacg/LICENSE.txt
 */

#ifndef PGIS_UTILITY_H
#define PGIS_UTILITY_H

#include "spdlog/spdlog.h"

#include "unistd.h"

#include <cerrno>
#include <cstring>
#include <string>

namespace {
inline std::string getHostname() {
  char *cName = (char *)malloc(255 * sizeof(char));
  if (!gethostname(cName, 255)) {
    spdlog::get("errconsole")->error("Unable to determine hostname");
  } else {
    spdlog::get("console")->debug("Running on host: {}", cName);
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
}  // namespace

#endif

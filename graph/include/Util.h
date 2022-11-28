/**
 * File: Util.h
 * License: Part of the MetaCG project. Licensed under BSD 3 clause license. See LICENSE.txt file at
 * https://github.com/tudasc/metacg/LICENSE.txt
 */

#ifndef METACG_UTIL_H
#define METACG_UTIL_H

#include "CgNode.h"
#include "LoggerUtil.h"
#include <set>
#include <string>



namespace metacg::util {

inline std::set<std::string> getFunctionNames(const std::unordered_set<CgNode *> &nodes) {
  std::set<std::string> names;
  for (const auto n : nodes) {
    names.emplace(n->getFunctionName());
  }
  return names;
}

inline std::vector<std::string> string_split(const std::string &in, const char c = '.') {
  std::vector<std::string::size_type> positions;
  positions.emplace_back(0);
  while (auto pos = in.find(c, positions.back() + 1)) {
    if (pos == std::string::npos) {
      break;
    }
    positions.push_back(pos + 1);
    if (positions.size() > 5000) {
      exit(-1);
    }
  }
  std::vector<std::string> vec;
  for (auto bg = positions.begin(); bg != positions.end(); ++bg) {
    auto end = bg + 1;
    vec.emplace_back(in.substr(*bg, (end - bg)));
  }
  return vec;
}

inline std::string extract_between(const std::string &s, const std::string &pattern, size_t &start) {
  size_t first = s.find(pattern, start) + pattern.size();
  size_t second = s.find(pattern, first);

  start = second + pattern.size();
  return s.substr(first, second - first);
}

inline int getVersionNoAtPosition(const std::string &versionStr, int index) {
  auto numOccurrences = std::count(versionStr.begin(), versionStr.end(), '.');
  if (numOccurrences < 1) {
    metacg::MCGLogger::instance().getErrConsole()->error("Could not interpret version string");
    exit(-1);
  }
  auto versionParts = string_split(versionStr);
  int version = std::atoi(versionParts.at(index).c_str());
  return version;
}

inline int getMajorVersionFromString(const std::string &versionStr) { return getVersionNoAtPosition(versionStr, 0); }

inline int getMinorVersionFromString(const std::string &versionStr) { return getVersionNoAtPosition(versionStr, 1); }

}  // namespace metacg::util

#endif  // METACG_UTIL_H

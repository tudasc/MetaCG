/**
* File: Util.h
* License: Part of the MetaCG project. Licensed under BSD 3 clause license. See LICENSE.txt file at
* https://github.com/tudasc/metacg/LICENSE.txt
*/

#ifndef METACG_UTIL_H
#define METACG_UTIL_H

#include "CgNode.h"
#include <set>
#include <string>

namespace metacg::util {

std::set<std::string> to_string(const std::set<CgNodePtr> &nodes) {
  std::set<std::string> names;
  for (const auto n : nodes) {
    names.emplace(n->getFunctionName());
  }
  return names;
}

}

#endif  // METACG_UTIL_H

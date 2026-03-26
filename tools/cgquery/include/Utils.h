/**
 * File: utils.h
 * License: Part of the MetaCG project. Licensed under BSD 3 clause license. See LICENSE.txt file at
 * https://github.com/tudasc/metacg/LICENSE.txt
 */
#pragma once
#include "metacg/Callgraph.h"
#include "metacg/CgNode.h"
#include "metacg/io/MCGReader.h"
#include <algorithm>
#include <memory>
#include <string>

enum ExitCode { Success = 0, Error = 1, Usage = 2 };

inline bool isNumber(const std::string& s) {
  return !s.empty() && std::all_of(s.begin(), s.end(), [](unsigned char c) { return std::isdigit(c); });
}

inline metacg::CgNode* findNode(metacg::Callgraph* cg, const std::string& spec, const std::string& role) {
  metacg::CgNode* node = nullptr;

  if (isNumber(spec)) {
    node = cg->getNode(std::stoul(spec));
  } else {
    if (cg->countNodes(spec) > 1) {
      metacg::MCGLogger::logWarn(role + " name '" + spec + "' is not unique; using first match.");
    }
    node = cg->getFirstNode(spec);
  }
  return node;
}

/**
 * File: PotentialFinalizer.h
 * License: Part of the MetaCG project. Licensed under BSD 3 clause license. See LICENSE.txt file at
 * https://github.com/tudasc/metacg/LICENSE.txt
 */

#pragma once

#include "Edge.h"

#include <string>
#include <vector>

struct potentialFinalizer {
  std::size_t argPos;
  std::string procedureCalled;
  std::vector<edge> finalizerEdges;

  explicit potentialFinalizer(std::size_t pos, std::string procCalled)
      : argPos(pos), procedureCalled(std::move(procCalled)) {}

  void addFinalizerEdge(const edge& e) { finalizerEdges.emplace_back(e); }
};

#pragma once

#include <string>
#include <vector>

#include "edge.h"

struct potentialFinalizer {
  std::size_t argPos;
  std::string procedureCalled;
  std::vector<edge> finalizerEdges;

  explicit potentialFinalizer(std::size_t pos, std::string procCalled)
      : argPos(pos), procedureCalled(std::move(procCalled)) {}

  void addFinalizerEdge(const edge& e) { finalizerEdges.emplace_back(e); }
};

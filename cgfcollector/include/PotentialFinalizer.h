/**
 * File: PotentialFinalizer.h
 * License: Part of the MetaCG project. Licensed under BSD 3 clause license. See LICENSE.txt file at
 * https://github.com/tudasc/metacg/LICENSE.txt
 */

#ifndef METACG_CGFCOLLECTOR_POTENTIALFINALIZER_H
#define METACG_CGFCOLLECTOR_POTENTIALFINALIZER_H

#include "Edge.h"

#include <string>
#include <vector>

namespace metacg::cgfcollector {

struct PotentialFinalizer {
  std::size_t argPos;
  std::string procedureCalled;
  std::vector<Edge> finalizerEdges;

  explicit PotentialFinalizer(std::size_t pos, std::string procCalled)
      : argPos(pos), procedureCalled(std::move(procCalled)) {}

  void addFinalizerEdge(const Edge& e) { finalizerEdges.emplace_back(e); }
};

}  // namespace metacg::cgfcollector

#endif  // METACG_CGFCOLLECTOR_POTENTIALFINALIZER_H

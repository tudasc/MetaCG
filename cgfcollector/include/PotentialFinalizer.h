/**
 * File: PotentialFinalizer.h
 * License: Part of the MetaCG project. Licensed under BSD 3 clause license. See LICENSE.txt file at
 * https://github.com/tudasc/metacg/LICENSE.txt
 */

#ifndef METACG_CGFCOLLECTOR_POTENTIALFINALIZER_H
#define METACG_CGFCOLLECTOR_POTENTIALFINALIZER_H

#include "Edge.h"

#include <flang/Semantics/symbol.h>
#include <string>
#include <vector>

namespace metacg::cgfcollector {

struct PotentialFinalizer {
  std::size_t argPos;
  std::vector<const Fortran::semantics::Symbol*> proceduresCalled;
  std::vector<EdgeSymbol> finalizerEdges;

  explicit PotentialFinalizer(std::size_t pos, std::vector<const Fortran::semantics::Symbol*> procsCalled)
      : argPos(pos), proceduresCalled(std::move(procsCalled)) {}

  void addFinalizerEdge(const EdgeSymbol& e) { finalizerEdges.emplace_back(e); }

  bool isInProceduresCalled(const Fortran::semantics::Symbol* procedure) const {
    return std::find_if(proceduresCalled.begin(), proceduresCalled.end(), [&](const Fortran::semantics::Symbol* p) {
             return compareSymbols(p, procedure);
           }) != proceduresCalled.end();
  }
};

}  // namespace metacg::cgfcollector

#endif  // METACG_CGFCOLLECTOR_POTENTIALFINALIZER_H

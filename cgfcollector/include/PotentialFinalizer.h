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
  // This list captures potential procedures that may have initialized a variable. Together with the variable tracking
  // information this is used to identify the generic implementation a variable has been initialized with.
  std::vector<const Fortran::semantics::Symbol*> potentialInitSite;
  std::vector<EdgeSymbol> finalizerEdges;
  const Fortran::semantics::Symbol* var;

  explicit PotentialFinalizer(std::size_t pos, std::vector<const Fortran::semantics::Symbol*> procsCalled,
                              const Fortran::semantics::Symbol* var)
      : argPos(pos), potentialInitSite(std::move(procsCalled)), var(var) {}

  void addFinalizerEdge(const EdgeSymbol& e) { finalizerEdges.emplace_back(e); }

  /**
   * @brief Checks whether a procedure symbol is a potential initialization site.
   *
   * @param procedure symbol
   * @return
   */
  bool isAPotentialInitSite(const Fortran::semantics::Symbol* procedure) const {
    return std::find_if(potentialInitSite.begin(), potentialInitSite.end(), [&](const Fortran::semantics::Symbol* p) {
             return compareSymbols(p, procedure);
           }) != potentialInitSite.end();
  }
};

}  // namespace metacg::cgfcollector

#endif  // METACG_CGFCOLLECTOR_POTENTIALFINALIZER_H

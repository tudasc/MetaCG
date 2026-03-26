/**
 * File: Edge.h
 * License: Part of the MetaCG project. Licensed under BSD 3 clause license. See LICENSE.txt file at
 * https://github.com/tudasc/metacg/LICENSE.txt
 */

#ifndef METACG_CGFCOLLECTOR_EDGE_H
#define METACG_CGFCOLLECTOR_EDGE_H

#include "FortranUtil.h"
#include "Type.h"

#include <LoggerUtil.h>
#include <flang/Parser/parse-tree.h>
#include <flang/Semantics/symbol.h>
#include <string>
#include <vector>

namespace metacg::cgfcollector {

struct PotentialFinalizer;

struct Edge {
  std::string caller;
  std::string callee;

  Edge(std::string caller, std::string callee) : caller(std::move(caller)), callee(std::move(callee)) {}

  bool operator==(const Edge& other) const { return caller == other.caller && callee == other.callee; }
  bool operator<(const Edge& other) const {
    return caller < other.caller || (caller == other.caller && callee < other.callee);
  }
};  // namespace metacg::cgfcollector

struct EdgeSymbol {
  const Fortran::semantics::Symbol* caller;
  const Fortran::semantics::Symbol* callee;

  EdgeSymbol(const Fortran::semantics::Symbol* caller, const Fortran::semantics::Symbol* callee)
      : caller(caller), callee(callee) {}

  bool operator==(const EdgeSymbol& other) const { return caller == other.caller && callee == other.callee; }
  bool operator<(const EdgeSymbol& other) const {
    return caller < other.caller || (caller == other.caller && callee < other.callee);
  }
};

struct EdgeManager {
  std::vector<Edge>& edges;

  EdgeManager(std::vector<Edge>& edges, bool underscoring) : edges(edges), underscoring(underscoring) {}

  void addEdge(const EdgeSymbol& e);
  void addEdge(const Fortran::semantics::Symbol* caller, const Fortran::semantics::Symbol* callee);
  void addEdge(const Edge& e);
  void addEdge(const std::string& caller, const std::string& callee);
  void addEdges(const std::vector<Edge>& newEdges);
  void addEdges(const std::vector<EdgeSymbol>& newEdges);

  /**
   * @brief For a given symbol, returns a list of edges from the current function to all finalizers of that type.
   *
   * @param types
   * @param currentFunctionSymbol
   * @param symbol
   */
  [[nodiscard]] std::vector<EdgeSymbol> getEdgesForFinalizers(const std::vector<Type>& types,
                                                              const Fortran::semantics::Symbol* currentFunctionSymbol,
                                                              const Fortran::semantics::Symbol* symbol);
  /**
   * @brief Calls getEdgesForFinalizers and adds them to the edges vector.
   *
   * @param types
   * @param currentFunctionSymbol
   * @param symbol
   */
  void addEdgesForFinalizers(const std::vector<Type>& types, const Fortran::semantics::Symbol* currentFunctionSymbol,
                             const Fortran::semantics::Symbol* symbol);

  void addEdgesForFinalizers(const PotentialFinalizer& e);

  /**
   * @brief Remove duplicate edges from the edges vector. This prevents massive amounts of duplication warnings from the
   * graph lib.
   */
  void uniquifyEdges();

 private:
  bool underscoring;
};

}  // namespace metacg::cgfcollector

#endif  // METACG_CGFCOLLECTOR_EDGE_H

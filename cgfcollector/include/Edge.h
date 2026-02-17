/**
 * File: Edge.h
 * License: Part of the MetaCG project. Licensed under BSD 3 clause license. See LICENSE.txt file at
 * https://github.com/tudasc/metacg/LICENSE.txt
 */

#pragma once

#include "FortranUtil.h"
#include "Type.h"

#include <LoggerUtil.h>
#include <flang/Parser/parse-tree.h>
#include <flang/Semantics/symbol.h>
#include <string>
#include <vector>

struct edge {
  std::string caller;
  std::string callee;

  edge(std::string caller, std::string callee) : caller(std::move(caller)), callee(std::move(callee)) {}

  bool operator==(const edge& other) const { return caller == other.caller && callee == other.callee; }
  bool operator<(const edge& other) const {
    return caller < other.caller || (caller == other.caller && callee < other.callee);
  }
};

struct edgeSymbol {
  const Fortran::semantics::Symbol* caller;
  const Fortran::semantics::Symbol* callee;

  edgeSymbol(const Fortran::semantics::Symbol* caller, const Fortran::semantics::Symbol* callee)
      : caller(caller), callee(callee) {}

  bool operator==(const edgeSymbol& other) const { return caller == other.caller && callee == other.callee; }
  bool operator<(const edgeSymbol& other) const {
    return caller < other.caller || (caller == other.caller && callee < other.callee);
  }
};

struct edgeManager {
  std::vector<edge>& edges;

  edgeManager(std::vector<edge>& edges) : edges(edges) {}

  void addEdge(const edgeSymbol& e, bool debug = true);
  void addEdge(const Fortran::semantics::Symbol* caller, const Fortran::semantics::Symbol* callee, bool debug = true);
  void addEdge(const edge& e, bool debug = true);
  void addEdge(const std::string& caller, const std::string& callee, bool debug = true);
  void addEdges(const std::vector<edge>& newEdges, bool debug = true);
  void addEdges(const std::vector<edgeSymbol>& newEdges, bool debug = true);

  /**
   * @brief For a given symbol, returns a list of edges from the current function to all finalizers of that type.
   *
   * @param types
   * @param currentFunctionSymbol
   * @param symbol
   */
  std::vector<edgeSymbol> getEdgesForFinalizers(const std::vector<type>& types,
                                                const Fortran::semantics::Symbol* currentFunctionSymbol,
                                                const Fortran::semantics::Symbol* symbol);
  /**
   * @brief Calls getEdgesForFinalizers and adds them to the edges vector.
   *
   * @param types
   * @param currentFunctionSymbol
   * @param symbol
   */
  void addEdgesForFinalizers(const std::vector<type>& types, const Fortran::semantics::Symbol* currentFunctionSymbol,
                             const Fortran::semantics::Symbol* symbol);
};

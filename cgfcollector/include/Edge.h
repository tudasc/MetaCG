/**
 * File: Edge.h
 * License: Part of the MetaCG project. Licensed under BSD 3 clause license. See LICENSE.txt file at
 * https://github.com/tudasc/metacg/LICENSE.txt
 */

#pragma once

#include <LoggerUtil.h>
#include <flang/Parser/parse-tree.h>
#include <flang/Semantics/symbol.h>
#include <string>
#include <vector>

#include "Type.h"
#include "Util.h"

using namespace Fortran::semantics;
using namespace Fortran::parser;
using namespace metacg;

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
  const Symbol* caller;
  const Symbol* callee;

  edgeSymbol(const Symbol* caller, const Symbol* callee) : caller(caller), callee(callee) {}

  bool operator==(const edgeSymbol& other) const { return caller == other.caller && callee == other.callee; }
  bool operator<(const edgeSymbol& other) const {
    return caller < other.caller || (caller == other.caller && callee < other.callee);
  }
};

struct edgeManager {
  std::vector<edge>& edges;

  edgeManager(std::vector<edge>& edges) : edges(edges) {}

  void addEdge(const edgeSymbol& e, bool debug = true);
  void addEdge(const Symbol* caller, const Symbol* callee, bool debug = true);
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
  std::vector<edgeSymbol> getEdgesForFinalizers(const std::vector<type>& types, const Symbol* currentFunctionSymbol,
                                                const Symbol* symbol);
  /**
   * @brief Calls getEdgesForFinalizers and adds them to the edges vector.
   *
   * @param types
   * @param currentFunctionSymbol
   * @param symbol
   */
  void addEdgesForFinalizers(const std::vector<type>& types, const Symbol* currentFunctionSymbol, const Symbol* symbol);
};

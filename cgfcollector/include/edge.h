#pragma once

#include <LoggerUtil.h>
#include <flang/Parser/parse-tree.h>
#include <flang/Semantics/symbol.h>
#include <string>
#include <vector>

#include "type.h"
#include "util.h"

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
  Symbol* caller;
  Symbol* callee;

  edgeSymbol(Symbol* caller, Symbol* callee) : caller(caller), callee(callee) {}

  bool operator==(const edgeSymbol& other) const { return caller == other.caller && callee == other.callee; }
  bool operator<(const edgeSymbol& other) const {
    return caller < other.caller || (caller == other.caller && callee < other.callee);
  }
};

struct edgeManager {
  std::vector<edge>& edges;

  edgeManager(std::vector<edge>& edges) : edges(edges) {}

  void addEdge(const edgeSymbol& e) { edges.emplace_back(mangleSymbol(e.caller), mangleSymbol(e.callee)); }
  void addEdge(const edge& e) { edges.emplace_back(e.caller, e.callee); }
  void addEdges(const std::vector<edge>& newEdges, bool debug = false) {
    for (const auto& e : newEdges) {
      edges.emplace_back(e.caller, e.callee);

      if (debug) {
        MCGLogger::logDebug("Add edge: {} -> {}", e.caller, e.callee);
      }
    }
  }
  void addEdges(const std::vector<edgeSymbol>& newEdges, bool debug = true) {
    for (const auto& e : newEdges) {
      edges.emplace_back(mangleSymbol(e.caller), mangleSymbol(e.callee));

      if (debug) {
        MCGLogger::logDebug("Add edge: {} ({}) -> {} ({})", mangleSymbol(e.caller), fmt::ptr(e.caller),
                            mangleSymbol(e.callee), fmt::ptr(e.callee));
      }
    }
  }

  static std::vector<edgeSymbol> getEdgesForFinalizers(std::vector<const type*> types,
                                                       const Symbol* currentFunctionSymbol);
  // void addEdgesForFinalizers(std::vector<const type*> types, const Symbol* currentFunctionSymbol);
};

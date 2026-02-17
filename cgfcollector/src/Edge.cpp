/**
 * File: Edge.cpp
 * License: Part of the MetaCG project. Licensed under BSD 3 clause license. See LICENSE.txt file at
 * https://github.com/tudasc/metacg/LICENSE.txt
 */

#include "Edge.h"

using namespace Fortran::semantics;
using namespace Fortran::parser;
using namespace metacg;

std::vector<EdgeSymbol> EdgeManager::getEdgesForFinalizers(const std::vector<Type>& types,
                                                           const Symbol* currentFunctionSymbol, const Symbol* symbol) {
  std::vector<EdgeSymbol> edges;

  std::vector<const Type*> typePtrs = findTypeWithDerivedTypes(types, symbol);

  for (const Type* type : typePtrs) {
    const Symbol* typeSymbol = type->typeSymbol;

    const DerivedTypeDetails* details = std::get_if<DerivedTypeDetails>(&typeSymbol->details());
    if (!details)
      continue;

    // add edges for finalizers
    for (const auto& final : details->finals()) {
      edges.emplace_back(currentFunctionSymbol, &final.second.get());
    }
  }

  return edges;
}

void EdgeManager::addEdgesForFinalizers(const std::vector<Type>& types, const Symbol* currentFunctionSymbol,
                                        const Symbol* symbol) {
  for (const EdgeSymbol& edge : getEdgesForFinalizers(types, currentFunctionSymbol, symbol)) {
    addEdge(edge, false);
    MCGLogger::logDebug("Add edge for finalizer: {} ({}) -> {} ({})", mangleSymbol(edge.caller), fmt::ptr(edge.caller),
                        mangleSymbol(edge.callee), fmt::ptr(edge.callee));
  }
}

void EdgeManager::addEdge(const EdgeSymbol& e, bool debug) {
  edges.emplace_back(mangleSymbol(e.caller), mangleSymbol(e.callee));
  if (debug) {
    MCGLogger::logDebug("Add edge: {} ({}) -> {} ({})", mangleSymbol(e.caller), fmt::ptr(e.caller),
                        mangleSymbol(e.callee), fmt::ptr(e.callee));
  }
}

void EdgeManager::addEdge(const Symbol* caller, const Symbol* callee, bool debug) {
  edges.emplace_back(mangleSymbol(caller), mangleSymbol(callee));
  if (debug) {
    MCGLogger::logDebug("Add edge: {} ({}) -> {} ({})", mangleSymbol(caller), fmt::ptr(caller), mangleSymbol(callee),
                        fmt::ptr(callee));
  }
}

void EdgeManager::addEdge(const Edge& e, bool debug) {
  edges.emplace_back(e.caller, e.callee);
  if (debug) {
    MCGLogger::logDebug("Add edge: {} -> {}", e.caller, e.callee);
  }
}

void EdgeManager::addEdge(const std::string& caller, const std::string& callee, bool debug) {
  edges.emplace_back(caller, callee);
  if (debug) {
    MCGLogger::logDebug("Add edge: {} -> {}", caller, callee);
  }
}

void EdgeManager::addEdges(const std::vector<Edge>& newEdges, bool debug) {
  for (const Edge& e : newEdges) {
    addEdge(e, debug);
  }
}

void EdgeManager::addEdges(const std::vector<EdgeSymbol>& newEdges, bool debug) {
  for (const EdgeSymbol& e : newEdges) {
    addEdge(e, debug);
  }
}

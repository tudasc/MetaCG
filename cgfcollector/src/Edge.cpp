/**
 * File: Edge.cpp
 * License: Part of the MetaCG project. Licensed under BSD 3 clause license. See LICENSE.txt file at
 * https://github.com/tudasc/metacg/LICENSE.txt
 */

#include "Edge.h"

#include "FortranUtil.h"
#include "PotentialFinalizer.h"

using namespace Fortran::semantics;
using namespace Fortran::parser;
using namespace metacg;

namespace metacg::cgfcollector {

std::vector<EdgeSymbol> EdgeManager::getEdgesForFinalizers(const std::vector<Type>& types,
                                                           const Symbol* currentFunctionSymbol, const Symbol* symbol) {
  std::vector<EdgeSymbol> edges;

  std::vector<const Type*> typePtrs = findTypeWithDerivedTypes(types, symbol);

  for (const Type* type : typePtrs) {
    const Symbol* typeSymbol = metacg::cgfcollector::canonicalizeSymbol(type->typeSymbol).symbol;
    const DerivedTypeDetails* details = typeSymbol->detailsIf<DerivedTypeDetails>();
    if (!details) {
      MCGLogger::logDebug("getEdgesForFinalizers: No DerivedTypeDetails for type: {} ({})", typeSymbol->name(),
                          fmt::ptr(typeSymbol));
      continue;
    }

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
    addEdge(edge);
    MCGLogger::logDebug("Add edge for finalizer: {} ({}) -> {} ({})", mangleSymbol(edge.caller, underscoring),
                        fmt::ptr(edge.caller), mangleSymbol(edge.callee, underscoring), fmt::ptr(edge.callee));
  }
}

void EdgeManager::addEdgesForFinalizers(const PotentialFinalizer& e) {
  for (const Edge& edge : e.finalizerEdges) {
    addEdge(edge);
    MCGLogger::logDebug("Add edge for finalizer: {} -> {}", edge.caller, edge.callee);
  }
}

void EdgeManager::addEdge(const EdgeSymbol& e) {
  edges.emplace_back(mangleSymbol(e.caller, underscoring), mangleSymbol(e.callee, underscoring));
  MCGLogger::logDebug("Add edge: {} ({}) -> {} ({})", mangleSymbol(e.caller, underscoring), fmt::ptr(e.caller),
                      mangleSymbol(e.callee, underscoring), fmt::ptr(e.callee));
}

void EdgeManager::addEdge(const Symbol* caller, const Symbol* callee) {
  edges.emplace_back(mangleSymbol(caller, underscoring), mangleSymbol(callee, underscoring));
  MCGLogger::logDebug("Add edge: {} ({}) -> {} ({})", mangleSymbol(caller, underscoring), fmt::ptr(caller),
                      mangleSymbol(callee, underscoring), fmt::ptr(callee));
}

void EdgeManager::addEdge(const Edge& e) {
  edges.emplace_back(e.caller, e.callee);
  MCGLogger::logDebug("Add edge: {} -> {}", e.caller, e.callee);
}

void EdgeManager::addEdge(const std::string& caller, const std::string& callee) {
  edges.emplace_back(caller, callee);
  MCGLogger::logDebug("Add edge: {} -> {}", caller, callee);
}

void EdgeManager::addEdges(const std::vector<Edge>& newEdges) {
  for (const Edge& e : newEdges) {
    addEdge(e);
  }
}

void EdgeManager::addEdges(const std::vector<EdgeSymbol>& newEdges) {
  for (const EdgeSymbol& e : newEdges) {
    addEdge(e);
  }
}

}  // namespace metacg::cgfcollector

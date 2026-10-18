/**
 * File: Edge.cpp
 * License: Part of the MetaCG project. Licensed under BSD 3 clause license. See LICENSE.txt file at
 * https://github.com/tudasc/metacg/LICENSE.txt
 */

#include "Edge.h"

#include "FortranUtil.h"
#include "PotentialFinalizer.h"
#include "metacg/LoggerUtil.h"

using namespace Fortran::semantics;
using namespace Fortran::parser;
using namespace metacg;

namespace metacg::cgfcollector {

void EdgeManager::addEdgesForFinalizers(
    const std::vector<Type>& types,
    const std::unordered_map<const Fortran::semantics::Symbol*, std::vector<const Fortran::semantics::Symbol*>>&
        finalizers,
    const Symbol* currentProcedureSymbol, const Symbol* symbol) {
  auto baseTypeIt = std::find_if(types.begin(), types.end(), [&](const Type& t) {
    return compareSymbols(t.typeSymbol, symbol, CanonicalMode::ByType);
  });
  if (baseTypeIt == types.end())
    return;

  try {
    auto final = finalizers.at(getAbsoluteBaseSymbol(types, &(*baseTypeIt)));
    for (const Symbol* f : final) {
      addEdge(currentProcedureSymbol, f);
    }
  } catch (const std::out_of_range& e) {
    // no finalizer for this type, do nothing
  }
}

void EdgeManager::addEdgesForFinalizers(const PotentialFinalizer& e) {
  for (const EdgeSymbol& edgeSym : e.finalizerEdges) {
    auto callee = canonicalizeSymbol(edgeSym.callee).symbol;
    addEdge(edgeSym.caller, callee);
    MCGLogger::logDebug("Add edge for finalizer: {} -> {}", mangleSymbol(edgeSym.caller, underscoring),
                        mangleSymbol(callee, underscoring));
  }
}

void EdgeManager::addEdge(const EdgeSymbol& e) { addEdge(e.caller, e.callee); }

void EdgeManager::addEdge(const Symbol* caller, const Symbol* callee) {
  if (const auto* gen = callee->detailsIf<Fortran::semantics::GenericDetails>()) {
    for (const auto& specificProc : gen->specificProcs()) {
      const Symbol* specificSymbol = &specificProc.get();
      edges.emplace_back(mangleSymbol(caller, underscoring), mangleSymbol(specificSymbol, underscoring));
      MCGLogger::logDebug("Add edge: {} ({}) ({}) -> {} ({}) ({})", mangleSymbol(caller, underscoring),
                          getDetailsName(caller), fmt::ptr(caller), mangleSymbol(specificSymbol, underscoring),
                          getDetailsName(specificSymbol), fmt::ptr(specificSymbol));
    }
  } else {
    callee = canonicalizeSymbol(callee).symbol;
    MCGLogger::logDebug("Add edge canonicalized: {} ({}) -> {} ({})", getDetailsName(caller), fmt::ptr(caller),
                        getDetailsName(callee), fmt::ptr(callee));
    edges.emplace_back(mangleSymbol(caller, underscoring), mangleSymbol(callee, underscoring));
    MCGLogger::logDebug("Add edge: {} ({}) ({}) -> {} ({}) ({})", mangleSymbol(caller, underscoring),
                        getDetailsName(caller), fmt::ptr(caller), mangleSymbol(callee, underscoring),
                        getDetailsName(callee), fmt::ptr(callee));
  }
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

void EdgeManager::uniquifyEdges() {
  std::sort(edges.begin(), edges.end());
  auto it = std::unique(edges.begin(), edges.end());
  edges.erase(it, edges.end());
}

}  // namespace metacg::cgfcollector

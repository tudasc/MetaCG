#include "edge.h"

std::vector<edgeSymbol> edgeManager::getEdgesForFinalizers(std::vector<type>& types,
                                                           const Symbol* currentFunctionSymbol, const Symbol* symbol) {
  std::vector<edgeSymbol> edges;

  std::vector<const type*> typePtrs = findTypeWithDerivedTypes(types, symbol);

  for (const type* type : typePtrs) {
    const Symbol* typeSymbol = type->typeSymbol;

    const auto* details = std::get_if<DerivedTypeDetails>(&typeSymbol->details());
    if (!details)
      continue;

    // add edges for finalizers
    for (const auto& final : details->finals()) {
      auto second = const_cast<Symbol*>(&final.second.get());  // TODO: remove const cast
      auto currentFunctionSymbolNoConst = const_cast<Symbol*>(currentFunctionSymbol);
      edges.emplace_back(currentFunctionSymbolNoConst, second);
    }
  }

  return edges;
}

void edgeManager::addEdgesForFinalizers(std::vector<type>& types, const Symbol* currentFunctionSymbol,
                                        const Symbol* symbol) {
  for (const auto& edge : getEdgesForFinalizers(types, currentFunctionSymbol, symbol)) {
    addEdge(edge, false);
    MCGLogger::logDebug("Add edge for finalizer: {} ({}) -> {} ({})", mangleSymbol(edge.caller), fmt::ptr(edge.caller),
                        mangleSymbol(edge.callee), fmt::ptr(edge.callee));
  }
}

void edgeManager::addEdge(const edgeSymbol& e, bool debug) {
  edges.emplace_back(mangleSymbol(e.caller), mangleSymbol(e.callee));
  if (debug) {
    MCGLogger::logDebug("Add edge: {} ({}) -> {} ({})", mangleSymbol(e.caller), fmt::ptr(e.caller),
                        mangleSymbol(e.callee), fmt::ptr(e.callee));
  }
}

void edgeManager::addEdge(Symbol* caller, Symbol* callee, bool debug) {
  edges.emplace_back(mangleSymbol(caller), mangleSymbol(callee));
  if (debug) {
    MCGLogger::logDebug("Add edge: {} ({}) -> {} ({})", mangleSymbol(caller), fmt::ptr(caller), mangleSymbol(callee),
                        fmt::ptr(callee));
  }
}

void edgeManager::addEdge(const edge& e, bool debug) {
  edges.emplace_back(e.caller, e.callee);
  if (debug) {
    MCGLogger::logDebug("Add edge: {} -> {}", e.caller, e.callee);
  }
}

void edgeManager::addEdge(const std::string& caller, const std::string& callee, bool debug) {
  edges.emplace_back(caller, callee);
  if (debug) {
    MCGLogger::logDebug("Add edge: {} -> {}", caller, callee);
  }
}

void edgeManager::addEdges(const std::vector<edge>& newEdges, bool debug) {
  for (const auto& e : newEdges) {
    addEdge(e, debug);
  }
}

void edgeManager::addEdges(const std::vector<edgeSymbol>& newEdges, bool debug) {
  for (const auto& e : newEdges) {
    addEdge(e, debug);
  }
}

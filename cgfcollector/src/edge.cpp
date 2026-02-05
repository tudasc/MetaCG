#include "edge.h"

/**
 * @brief For a given type symbol, returns a list of edges from the current function to all finalizers of that type.
 *
 * @param typeSymbol
 */
std::vector<edgeSymbol> edgeManager::getEdgesForFinalizers(std::vector<const type*> types,
                                                           const Symbol* currentFunctionSymbol) {
  std::vector<edgeSymbol> edges;
  // std::vector<const type*> typeSymbols = findTypeWithDerivedTypes(typeSymbol);

  for (const type* type : types) {
    const Symbol* typeSymbol = type->type;

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

// void edgeManager::addEdgesForFinalizers(std::vector<const type*> types, const Symbol* currentFunctionSymbol) {
//   for (const auto& edge : getEdgesForFinalizers(types, currentFunctionSymbol)) {
//     edges.emplace_back(mangleSymbol(edge.caller), mangleSymbol(edge.callee));

//     MCGLogger::logDebug("Add edge for finalizer: {} ({}) -> {} ({})", mangleSymbol(edge.caller),
//     fmt::ptr(edge.caller),
//                         mangleSymbol(edge.callee), fmt::ptr(edge.callee));
//   }
// }

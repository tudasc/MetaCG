/**
 * File: Util.h
 * License: Part of the MetaCG project. Licensed under BSD 3 clause license. See LICENSE.txt file at
 * https://github.com/tudasc/metacg/LICENSE.txt
 */

#pragma once

#include "LoggerUtil.h"
#include <cassert>
#include <flang/Lower/Mangler.h>
#include <flang/Optimizer/Support/InternalNames.h>
#include <flang/Parser/parse-tree.h>
#include <flang/Semantics/symbol.h>

#include "Type.h"

using namespace Fortran::parser;
using namespace Fortran::semantics;
using namespace Fortran::common;
using namespace metacg;

/**
 * @brief Formatter for CharBlock
 */
template <>
struct fmt::formatter<Fortran::parser::CharBlock> {
  constexpr auto parse(fmt::format_parse_context& ctx) { return ctx.begin(); }

  template <typename FormatContext>
  auto format(const Fortran::parser::CharBlock& cb, FormatContext& ctx) const {
    return fmt::format_to(ctx.out(), "{}", std::string_view(cb.begin(), cb.size()));
  }
};

/**
 * @brief Variant check if it holds any of the given types
 *
 * @tparam Variant
 * @tparam Ts
 * @param v
 * @return
 */
template <typename Variant, typename... Ts>
bool holds_any_of(const Variant& v) {
  return (std::holds_alternative<Ts>(v) || ...);
}

/**
 * @brief Get Name from type that has a designator
 *
 * @tparam T
 * @param t
 * @return
 */
template <typename T>
const Name* getNameFromClassWithDesignator(const T& t) {
  if (const Indirection<Designator>* designator = std::get_if<Indirection<Designator>>(&t.u)) {
    if (const DataRef* dataRef = std::get_if<DataRef>(&designator->value().u)) {
      if (const Name* name = std::get_if<Name>(&dataRef->u)) {
        return name;
      }
    }
  }
  return nullptr;
}

/**
 * @brief Compares two symbols for equality also resolves the original construct the symbol comes from.
 * This could have been defined in another module/file
 *
 * @param a
 * @param b
 * @return true if both symbols are equal
 */
bool compareSymbols(const Symbol* a, const Symbol* b);

/**
 * @brief Generate mangled name from symbol
 *
 * @param sym
 * @return
 */
std::string mangleSymbol(const Symbol* sym);

/**
 * @brief Check if expression is an operator expression
 *
 * @param e
 * @return
 */
bool isOperator(const Expr* e);

/**
 * @brief Compare if expression match given intrinsic operator
 *
 * @param expr
 * @param op
 * @return
 */
bool compareExprIntrinsicOperator(const Expr* expr, DefinedOperator::IntrinsicOperator op);

/**
 * @brief Check if binary operator expression
 *
 * @param e
 * @return
 */
bool isBinaryOperator(const Expr* e);

/**
 * @brief Check if unary operator expression
 *
 * @param e
 * @return
 */
bool isUnaryOperator(const Expr* e);

/**
 * @brief Get intrinsic operator from a variant of several operator types (RelationalOperator, LogicalOperator,
 * NumericOperator)
 *
 * @tparam Variant
 * @param op
 * @return
 */
DefinedOperator::IntrinsicOperator variantGetIntrinsicOperator(const GenericKind& gk);

/**
 * @brief Get type symbol from a given symbol
 *
 * @param symbol
 * @return
 */
const Symbol* getTypeSymbolFromSymbol(const Symbol* symbol);

/**
 * @brief Searches the types vector for given symbol and returns pointers to vectors of type with derived types. The
 * type symbol is derived from the given symbol.
 *
 * @param typeSymbol symbol to search for
 * @return vector with type and all derived types
 */
std::vector<const type*> findTypeWithDerivedTypes(const std::vector<type>& types, const Symbol* symbol);

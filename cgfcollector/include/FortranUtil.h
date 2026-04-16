/**
 * File: FortranUtil.h
 * License: Part of the MetaCG project. Licensed under BSD 3 clause license. See LICENSE.txt file at
 * https://github.com/tudasc/metacg/LICENSE.txt
 */

#ifndef METACG_CGFCOLLECTOR_FORTRANUTIL_H
#define METACG_CGFCOLLECTOR_FORTRANUTIL_H

#include "Type.h"

#include <cassert>
#include <flang/Lower/Mangler.h>
#include <flang/Optimizer/Support/InternalNames.h>
#include <flang/Parser/parse-tree.h>
#include <flang/Semantics/symbol.h>
#include <metacg/LoggerUtil.h>

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

namespace metacg::cgfcollector {

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
const Fortran::parser::Name* getNameFromClassWithDesignator(const T& t) {
  if (const Fortran::common::Indirection<Fortran::parser::Designator>* designator =
          std::get_if<Fortran::common::Indirection<Fortran::parser::Designator>>(&t.u)) {
    if (const Fortran::parser::DataRef* dataRef = std::get_if<Fortran::parser::DataRef>(&designator->value().u)) {
      if (const Fortran::parser::Name* name = std::get_if<Fortran::parser::Name>(&dataRef->u)) {
        return name;
      }
    }
  }
  return nullptr;
}

/**
 * @brief Get the derived type symbol from a symbol that has a type.
 *
 * @param sym
 */
const Fortran::semantics::Symbol* getTypeAsDerivedTypeSymbol(const Fortran::semantics::Symbol* sym);

/**
 * @brief Get the absolute base symbol for a given type. This is done by following the `extendsFrom` field of the type
 * until the `extendsFrom` field is null. This also canonicalizes the resulting symbol.
 *
 * @param types
 * @param type
 */
const Fortran::semantics::Symbol* getAbsoluteBaseSymbol(const std::vector<Type>& types, const Type* type);

enum class CanonicalMode {
  Identity,  // preserve variables/procedures by symbol
  ByType,    // normalize variables/functions by derived type
};

/**
 * @class CanonicalSymbol
 * @brief Struct to hold symbol and its canonical kind. Mainly used for comparing symbols.
 *
 */
struct CanonicalSymbol {
  enum class Kind { DerivedType, Procedure, Other };

  const Fortran::semantics::Symbol* symbol;
  Kind kind;
};

/**
 * @brief Canonicalizes a given symbol. This tries to resolve a symbol to its original construct, e.g. if it's defined
 * in a modules and therefore is a Use/Host associated symbol, it will be resolved to the original symbol in the module.
 * This function also handles generic symbols, and an short coming of older LLVM versions (18 and before). See function
 * body for details.
 *
 * Note: Mainly used for comparing symbols.
 *
 * @param input
 * @param mode (see CanonicalMode definition)
 * @return
 */
CanonicalSymbol canonicalizeSymbol(const Fortran::semantics::Symbol* input,
                                   CanonicalMode mode = CanonicalMode::Identity);

/**
 * @brief Compares two symbols for equality also resolves the original construct the symbol comes from.
 * This could have been defined in another module/file.
 *
 * @param a
 * @param b
 * @param mode (see CanonicalMode definition)
 * @return true if both symbols are equal
 */
bool compareSymbols(const Fortran::semantics::Symbol* a, const Fortran::semantics::Symbol* b,
                    CanonicalMode mode = CanonicalMode::Identity);

/**
 * @brief Generate mangled name from symbol
 *
 * @param sym
 * @param underscoring
 * @return
 */
std::string mangleSymbol(const Fortran::semantics::Symbol* sym, bool underscoring);

/**
 * @brief Check if expression is an operator expression
 *
 * @param e
 * @return
 */
bool isOperator(const Fortran::parser::Expr* e);

/**
 * @brief Compare if expression match given intrinsic operator
 *
 * @param expr
 * @param op
 * @return
 */
bool compareExprIntrinsicOperator(const Fortran::parser::Expr* expr,
                                  Fortran::parser::DefinedOperator::IntrinsicOperator op);

/**
 * @brief Check if binary operator expression
 *
 * @param e
 * @return
 */
bool isBinaryOperator(const Fortran::parser::Expr* e);

/**
 * @brief Check if unary operator expression
 *
 * @param e
 * @return
 */
bool isUnaryOperator(const Fortran::parser::Expr* e);

/**
 * @brief Get intrinsic operator from a variant of several operator types (RelationalOperator, LogicalOperator,
 * NumericOperator)
 *
 * @param gk
 * @return
 */
Fortran::parser::DefinedOperator::IntrinsicOperator variantGetIntrinsicOperator(
    const Fortran::semantics::GenericKind& gk);

/**
 * @brief First, searches the provided `types` vector for a type that references the given `symbol`. Then, for that
 * type, searches the `types` vector again to find all types that either extend from it or are derived from it. This
 * search is done in two parts: First, all descendants of the type are searched, then all ancestors.
 *
 * This is used to handle polymorphic calls. If a type is used in a call, it could be any type that extends from it or
 * is extended from it.
 *
 * NOTE: Possible improvement: Use a Map [Procedure -> List of Procedures that are overridden]
 *
 * @param types
 * @param Symbol symbol to search for
 * @return vector with original type and all types that are derived from it or extend from it. If no type is found with
 * the given symbol, an empty vector is returned.
 */
std::vector<const Type*> findTypeWithDerivedTypes(const std::vector<Type>& types,
                                                  const Fortran::semantics::Symbol* symbol);

std::string getDetailsName(const Fortran::semantics::Symbol* symbol);

}  // namespace metacg::cgfcollector

#endif  // !METACG_CGFCOLLECTOR_FORTRANUTIL_H

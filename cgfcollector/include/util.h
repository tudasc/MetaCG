#pragma once

#include "LoggerUtil.h"
#include <cassert>
#include <flang/Lower/Mangler.h>
#include <flang/Optimizer/Support/InternalNames.h>
#include <flang/Parser/parse-tree.h>
#include <flang/Semantics/symbol.h>

using namespace Fortran::parser;
using namespace Fortran::semantics;
using namespace Fortran::common;
using namespace metacg;

template <>
struct fmt::formatter<Fortran::parser::CharBlock> {
  constexpr auto parse(fmt::format_parse_context& ctx) { return ctx.begin(); }

  template <typename FormatContext>
  auto format(const Fortran::parser::CharBlock& cb, FormatContext& ctx) const {
    return fmt::format_to(ctx.out(), "{}", std::string_view(cb.begin(), cb.size()));
  }
};

template <typename Variant, typename... Ts>
bool holds_any_of(const Variant& v) {
  return (std::holds_alternative<Ts>(v) || ...);
}

bool compareSymbols(const Symbol* a, const Symbol* b);
std::string mangleSymbol(const Symbol* sym);
bool isOperator(const Expr* e);
bool compareExprIntrinsicOperator(const Expr* expr, DefinedOperator::IntrinsicOperator op);
bool isBinaryOperator(const Expr* e);
bool isUnaryOperator(const Expr* e);

// map RelationalOperator, LogicalOperator, NumericOperator to DefinedOperator::IntrinsicOperator
template <typename Variant>
DefinedOperator::IntrinsicOperator variantGetIntrinsicOperator(const Variant& op);

const Symbol* getTypeSymbolFromSymbol(const Symbol* symbol);

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

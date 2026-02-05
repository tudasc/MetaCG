#pragma once

#include <LoggerUtil.h>
#include <flang/Parser/parse-tree.h>
#include <flang/Semantics/symbol.h>
#include <unordered_set>
#include <vector>

using namespace Fortran::semantics;
using namespace Fortran::parser;
using namespace metacg;

struct type {
  Symbol* typeSymbol;
  Symbol* extendsFrom;
  std::vector<std::pair<Symbol*, Symbol*>> procedures;                            // name(symbol) => optname(symbol)
  std::vector<std::pair<DefinedOperator::IntrinsicOperator, Symbol*>> operators;  // operator => name(symbol)
};

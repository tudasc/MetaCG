/**
 * File: Type.h
 * License: Part of the MetaCG project. Licensed under BSD 3 clause license. See LICENSE.txt file at
 * https://github.com/tudasc/metacg/LICENSE.txt
 */

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
  const Symbol* typeSymbol;
  const Symbol* extendsFrom;
  std::vector<std::pair<const Symbol*, const Symbol*>> procedures;  // name(symbol) => optname(symbol)
  std::vector<std::pair<DefinedOperator::IntrinsicOperator, const Symbol*>> operators;  // operator => name(symbol)
};

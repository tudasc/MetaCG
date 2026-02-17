/**
 * File: Type.h
 * License: Part of the MetaCG project. Licensed under BSD 3 clause license. See LICENSE.txt file at
 * https://github.com/tudasc/metacg/LICENSE.txt
 */

#pragma once

#include <LoggerUtil.h>
#include <flang/Parser/parse-tree.h>
#include <flang/Semantics/symbol.h>
#include <vector>

struct Type {
  const Fortran::semantics::Symbol* typeSymbol;
  const Fortran::semantics::Symbol* extendsFrom;
  std::vector<std::pair<const Fortran::semantics::Symbol*, const Fortran::semantics::Symbol*>>
      procedures;  // name(symbol) => optname(symbol)
  std::vector<std::pair<Fortran::parser::DefinedOperator::IntrinsicOperator, const Fortran::semantics::Symbol*>>
      operators;  // operator => name(symbol)
};

/**
 * File: Function.h
 * License: Part of the MetaCG project. Licensed under BSD 3 clause license. See LICENSE.txt file at
 * https://github.com/tudasc/metacg/LICENSE.txt
 */

#pragma once

#include <flang/Semantics/symbol.h>
#include <vector>

struct function {
  struct dummyArg {
    const Fortran::semantics::Symbol* symbol;
    bool hasBeenInitialized = false;

    explicit dummyArg(const Fortran::semantics::Symbol* sym) : symbol(sym) {}
    explicit dummyArg(const Fortran::semantics::Symbol* sym, bool init) : symbol(sym), hasBeenInitialized(init) {}
  };

  const Fortran::semantics::Symbol* symbol;  // function symbol
  std::vector<dummyArg> dummyArgs;

  explicit function(const Fortran::semantics::Symbol* sym) : symbol(sym) {}
  explicit function(const Fortran::semantics::Symbol* sym, std::vector<dummyArg> args)
      : symbol(sym), dummyArgs(std::move(args)) {}

  void addDummyArg(const Fortran::semantics::Symbol* sym) { dummyArgs.emplace_back(sym); }
};

/**
 * File: Function.h
 * License: Part of the MetaCG project. Licensed under BSD 3 clause license. See LICENSE.txt file at
 * https://github.com/tudasc/metacg/LICENSE.txt
 */

#pragma once

#include <flang/Semantics/symbol.h>
#include <vector>

namespace metacg::cgfcollector {

struct Function {
  struct DummyArg {
    const Fortran::semantics::Symbol* symbol;
    bool hasBeenInitialized = false;

    explicit DummyArg(const Fortran::semantics::Symbol* sym) : symbol(sym) {}
    explicit DummyArg(const Fortran::semantics::Symbol* sym, bool init) : symbol(sym), hasBeenInitialized(init) {}
  };

  const Fortran::semantics::Symbol* symbol;  // function symbol
  std::vector<DummyArg> dummyArgs;

  explicit Function(const Fortran::semantics::Symbol* sym) : symbol(sym) {}
  explicit Function(const Fortran::semantics::Symbol* sym, std::vector<DummyArg> args)
      : symbol(sym), dummyArgs(std::move(args)) {}

  void addDummyArg(const Fortran::semantics::Symbol* sym) { dummyArgs.emplace_back(sym); }
};

}  // namespace metacg::cgfcollector

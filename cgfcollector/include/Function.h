/**
 * File: Function.h
 * License: Part of the MetaCG project. Licensed under BSD 3 clause license. See LICENSE.txt file at
 * https://github.com/tudasc/metacg/LICENSE.txt
 */

#pragma once

#include <flang/Semantics/symbol.h>
#include <vector>

using namespace Fortran::semantics;

struct function {
  struct dummyArg {
    const Symbol* symbol;
    bool hasBeenInitialized = false;

    explicit dummyArg(const Symbol* sym) : symbol(sym) {}
    explicit dummyArg(const Symbol* sym, bool init) : symbol(sym), hasBeenInitialized(init) {}
  };

  const Symbol* symbol;  // function symbol
  std::vector<dummyArg> dummyArgs;

  explicit function(const Symbol* sym) : symbol(sym) {}
  explicit function(const Symbol* sym, std::vector<dummyArg> args) : symbol(sym), dummyArgs(std::move(args)) {}

  void addDummyArg(const Symbol* sym) { dummyArgs.emplace_back(sym); }
};

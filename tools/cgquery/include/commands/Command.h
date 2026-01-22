/**
 * File: Command.h
 * License: Part of the MetaCG project. Licensed under BSD 3 clause license. See LICENSE.txt file at
 * https://github.com/tudasc/metacg/LICENSE.txt
 */
#pragma once
#include "Callgraph.h"
#include <string>
#include <vector>

struct Command {
  virtual ~Command() = default;
  virtual int run(const std::vector<char*>& args) = 0;
  virtual std::string name() const = 0;

 protected:
  metacg::Callgraph* cg;
};

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

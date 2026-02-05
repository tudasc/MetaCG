#include <flang/Semantics/symbol.h>
#include <vector>

using namespace Fortran::semantics;

struct function {
  struct dummyArg {
    Symbol* symbol;
    bool hasBeenInitialized = false;

    explicit dummyArg(Symbol* sym) : symbol(sym) {}
    explicit dummyArg(Symbol* sym, bool init) : symbol(sym), hasBeenInitialized(init) {}
  };

  Symbol* symbol;  // function symbol
  std::vector<dummyArg> dummyArgs;

  explicit function(Symbol* sym) : symbol(sym) {}
  explicit function(Symbol* sym, std::vector<dummyArg> args) : symbol(sym), dummyArgs(std::move(args)) {}

  void addDummyArg(Symbol* sym) { dummyArgs.emplace_back(sym); }
};

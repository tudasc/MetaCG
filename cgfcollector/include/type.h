#include <flang/Parser/parse-tree.h>
#include <flang/Semantics/symbol.h>
#include <vector>

using namespace Fortran::semantics;
using namespace Fortran::parser;

struct type {
  Symbol* type;
  Symbol* extendsFrom;
  std::vector<std::pair<Symbol*, Symbol*>> procedures;                            // name(symbol) => optname(symbol)
  std::vector<std::pair<DefinedOperator::IntrinsicOperator, Symbol*>> operators;  // operator => name(symbol)
};

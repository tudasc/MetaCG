#include <flang/Semantics/symbol.h>

using namespace Fortran::semantics;

struct trackedVar {
  Symbol* var;
  Symbol* procedure;  // procedure in which var was defined
  bool hasBeenInitialized = false;
  bool addFinalizers = false;
};

#pragma once

#include <cassert>
#include <flang/Lower/Mangler.h>
#include <flang/Optimizer/Support/InternalNames.h>
#include <flang/Semantics/symbol.h>

using namespace Fortran::parser;
using namespace Fortran::semantics;
using namespace Fortran::common;
using Fortran::lower::mangle::mangleName;

bool compareSymbols(const Symbol* a, const Symbol* b);
std::string mangleSymbol(const Symbol* sym);

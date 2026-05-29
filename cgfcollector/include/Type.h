/**
 * File: Type.h
 * License: Part of the MetaCG project. Licensed under BSD 3 clause license. See LICENSE.txt file at
 * https://github.com/tudasc/metacg/LICENSE.txt
 */

#ifndef METACG_CGFCOLLECTOR_TYPE_H
#define METACG_CGFCOLLECTOR_TYPE_H

#include <flang/Parser/parse-tree.h>
#include <flang/Semantics/symbol.h>
#include <metacg/LoggerUtil.h>
// #include <vector>

namespace metacg::cgfcollector {

struct Type {
  const Fortran::semantics::Symbol* typeSymbol;
  const Fortran::semantics::Symbol* extendsFrom;
};

}  // namespace metacg::cgfcollector

#endif  // METACG_CGFCOLLECTOR_TYPE_H

/**
* File: ASTUtil.h
* License: Part of the MetaCG project. Licensed under BSD 3 clause license. See LICENSE.txt file at
* https://github.com/tudasc/metacg/LICENSE.txt
*/

#ifndef CGCOLLECTOR2_ASTUTIL_H
#define CGCOLLECTOR2_ASTUTIL_H

#include <clang/AST/ExprCXX.h>

std::vector<std::string> getMangledNames(clang::Decl const* const nd);

#endif  // CGCOLLECTOR2_ASTUTIL_H

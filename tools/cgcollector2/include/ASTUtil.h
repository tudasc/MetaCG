/**
* File: ASTUtil.h
* License: Part of the MetaCG project. Licensed under BSD 3 clause license. See LICENSE.txt file at
* https://github.com/tudasc/metacg/LICENSE.txt
*/

#ifndef CGCOLLECTOR2_ASTUTIL_H
#define CGCOLLECTOR2_ASTUTIL_H

#include <clang/AST/ExprCXX.h>
#include <clang/AST/Mangle.h>

const clang::Type* resolveToUnderlyingType(const clang::Type* ty);
std::vector<std::string> mangleCtorDtor(const clang::FunctionDecl* const nd, clang::MangleContext* mc);
std::vector<std::string> getMangledNames(clang::Decl const* const nd);
clang::Stmt* getCalledStmtFromCXXMemberCall(clang::CXXMemberCallExpr* MCE);
clang::FunctionDecl* getCalledFunctionFromCXXOperatorCallExpr(clang::CXXOperatorCallExpr* OCE);
std::pair<clang::Stmt*, bool> getImplicitObjectFromCXXMemberCall(clang::CXXMemberCallExpr* MCE);
void printNamedDeclToConsole(const clang::Decl* D);
auto Location(clang::Decl* decl, clang::Stmt* expr);

#endif  // CGCOLLECTOR2_ASTUTIL_H

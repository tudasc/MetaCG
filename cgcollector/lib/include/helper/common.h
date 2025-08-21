/**
* File: common.h
* License: Part of the MetaCG project. Licensed under BSD 3 clause license. See LICENSE.txt file at
* https://github.com/tudasc/metacg/LICENSE.txt
 */
#ifndef CGCOLLECTOR_HELPER_COMMON_H
#define CGCOLLECTOR_HELPER_COMMON_H

#include <clang/AST/ExprCXX.h>
#include <clang/AST/Mangle.h>

#include <string>
#include <vector>

/**
 * Returns mangled names for all named decls, including Ctor/Dtor.
 */
std::vector<std::string> getMangledName(clang::NamedDecl const* const nd);

/**
 * Returns the called Statement from a CXXMemberCall
 * For example if the call is '(a.*(&A::foo))();' the returned Statement will be (&A::foo)
 * @param MCE
 * @return The called Statement or a nullptr if it can not be determined
 */
clang::Stmt* getCalledStmtFromCXXMemberCall(clang::CXXMemberCallExpr* MCE);

/**
 * Returns the implicit object from a CXXMemberCall
 * For example if the call is '(a.*(&A::foo))();' the returned Statement will be 'a'
 * @param MCE
 * @return The implicit object (or a nullptr if it can not be determined) and 'True' if the CXXMemberCall is an arrow
 * operator
 */
std::pair<clang::Stmt*, bool> getImplicitObjectFromCXXMemberCall(clang::CXXMemberCallExpr* MCE);

/**
 * Returns the declaration of the function that gets ultimately called by a CXXOperatorCallExpr.
 * This function can either be a normal, freestanding function or a CXXMethodDecl
 * If it is a CXXMethodDecl, the first argument of the CXXOperatorCallExpr is the implicit object (*this)
 * @param OCE
 * @return The called function Decl or a nullptr if it can not be determined
 */
clang::FunctionDecl* getCalledFunctionFromCXXOperatorCallExpr(clang::CXXOperatorCallExpr* OCE);

#endif

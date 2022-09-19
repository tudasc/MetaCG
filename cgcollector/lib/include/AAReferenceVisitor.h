/**
 * File: AAReferenceVisitor.h
 * License: Part of the MetaCG project. Licensed under BSD 3 clause license. See LICENSE.txt file at
 * https://github.com/tudasc/metacg/LICENSE.txt
 */

#ifndef CGCOLLECTOR_AAREFERENCEVISITOR_H
#define CGCOLLECTOR_AAREFERENCEVISITOR_H

#include "AliasAnalysis.h"

namespace implementation {
VectorType<ObjectNameDereference> getReferencedDecls(clang::Stmt *Stmt, const clang::ASTContext *CTX,
                                                     clang::FunctionDecl *ParentFunctionDecl);
VectorType<StringType> getReferencedDeclsStr(clang::Stmt *Stmt, const clang::ASTContext *CTX,
                                             clang::FunctionDecl *ParentFunctionDecl);
VectorType<StringType> getDecls(clang::Decl *Decl, const clang::ASTContext *CTX);
VectorType<StringType> getReferencedInReturnStmts(clang::Stmt *Stmt, const clang::ASTContext *CTX,
                                                  clang::FunctionDecl *ParentFunctionDecl);

}  // namespace implementation

#endif  // CGCOLLECTOR_AAREFERENCEVISITOR_H

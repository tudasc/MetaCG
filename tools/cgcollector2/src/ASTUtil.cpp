/**
* File: ASTUtil.cpp
* License: Part of the MetaCG project. Licensed under BSD 3 clause license. See LICENSE.txt file at
* https://github.com/tudasc/metacg/LICENSE.txt
*/

#include "ASTUtil.h"

#include <clang/AST/Mangle.h>
#include <iostream>

#include <clang/AST/DeclObjC.h>

#include "SharedDefs.h"

const clang::Type* resolveToUnderlyingType(const clang::Type* ty) {
  if (!ty->isPointerType()) {
    return ty;
    // std::cout << "resolveToUnderlyingType: isPointerType" << std::endl;
  }
  return resolveToUnderlyingType(ty->getPointeeType().getTypePtr());
}


std::vector<std::string> getMangledNames(clang::Decl const* const nd) {
  if (!nd) {
    std::cerr << "NamedDecl was nullptr" << std::endl;
    assert(nd && "NamedDecl and MangleContext must not be nullptr");
    return {"__NO_NAME__"};
  }
  clang::ASTNameGenerator NG(nd->getASTContext());
  if (llvm::isa<clang::CXXRecordDecl>(nd) || llvm::isa<clang::CXXMethodDecl>(nd) ||
      llvm::isa<clang::ObjCInterfaceDecl>(nd) || llvm::isa<clang::ObjCImplementationDecl>(nd)) {
    return NG.getAllManglings(nd);
  }
  return {NG.getName(nd)};
}

clang::Stmt* getCalledStmtFromCXXMemberCall(clang::CXXMemberCallExpr* MCE) {
  clang::Expr* Callee = MCE->getCallee()->IgnoreParens();
  if (auto* MemExpr = llvm::dyn_cast<clang::MemberExpr>(Callee)) {
    return MemExpr;
  }
  if (auto* BO = llvm::dyn_cast<clang::BinaryOperator>(Callee)) {
    if (BO->getOpcode() == clang::BO_PtrMemD || BO->getOpcode() == clang::BO_PtrMemI) {
      return BO->getRHS();
    }
  }
  return nullptr;
}

clang::FunctionDecl* getCalledFunctionFromCXXOperatorCallExpr(clang::CXXOperatorCallExpr* OCE) {
  auto Called = OCE->getCallee()->IgnoreParenCasts();
  if (auto* DeclRefExpr = llvm::dyn_cast<clang::DeclRefExpr>(Called)) {
    if (auto* FunctionDecl = llvm::dyn_cast<clang::FunctionDecl>(DeclRefExpr->getDecl())) {
      return FunctionDecl;
    }
  }
  return nullptr;
}

std::pair<clang::Stmt*, bool> getImplicitObjectFromCXXMemberCall(clang::CXXMemberCallExpr* MCE) {
  clang::Expr* Callee = MCE->getCallee()->IgnoreParens();
  if (const auto* MemExpr = llvm::dyn_cast<clang::MemberExpr>(Callee))
    return {MemExpr->getBase(), MemExpr->isArrow()};
  if (const auto* BO = llvm::dyn_cast<clang::BinaryOperator>(Callee)) {
    if (BO->getOpcode() == clang::BO_PtrMemD) {
      return {BO->getLHS(), false};
    } else if (BO->getOpcode() == clang::BO_PtrMemI) {
      return {BO->getLHS(), true};
    }
  }

  return {nullptr, false};
}

void printNamedDeclToConsole(const clang::Decl* D) {
  assert(D);
#ifndef NDEBUG
  auto fd = llvm::dyn_cast<clang::NamedDecl>(D);
  if (fd) {
    std::cout << fd->getNameAsString() << std::endl;
  }
#endif
}

auto getLocation(clang::Decl* decl, clang::Stmt* expr) {
  const auto& ctx = decl->getASTContext();
  const auto& man = ctx.getSourceManager();
  return expr->getSourceRange().getBegin().printToString(man);
}
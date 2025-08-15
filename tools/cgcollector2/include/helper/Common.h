/**
* File: Common.h
* License: Part of the MetaCG project. Licensed under BSD 3 clause license. See LICENSE.txt file at
* https://github.com/tudasc/metacg/LICENSE.txt
 */


#ifndef CGCOLLECTOR2_HELPER_COMMON_H
#define CGCOLLECTOR2_HELPER_COMMON_H

#include <clang/AST/ExprCXX.h>
#include <clang/AST/Mangle.h>

#include <clang/AST/DeclCXX.h>
#include <clang/AST/DeclObjC.h>

#include <string>
#include <vector>

/**
 * Returns mangled names for all named decls, including Ctor/Dtor.
 */
std::vector<std::string> getMangledName(clang::NamedDecl const* const nd) {
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

#endif

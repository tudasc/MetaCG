#include "helper/common.h"
#include "clang/AST/Mangle.h"


std::string getMangledName(clang::NamedDecl const *const nd) {
  clang::ASTNameGenerator ang(nd->getASTContext());
  return ang.getName(nd);
}

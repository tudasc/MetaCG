
#ifndef METACG_CGCOLLECTOR_UTILS_H
#define METACG_CGCOLLECTOR_UTILS_H

#include <clang/AST/Type.h>

const clang::Type* resolveToUnderlyingType(const clang::Type* ty) {
  if (!ty->isPointerType()) {
    return ty;
    // std::cout << "resolveToUnderlyingType: isPointerType" << std::endl;
  }
  return resolveToUnderlyingType(ty->getPointeeType().getTypePtr());
}

#endif

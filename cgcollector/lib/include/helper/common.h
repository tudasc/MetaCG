#ifndef CGCOLLECTOR_HELPER_COMMOM_H
#define CGCOLLECTOR_HELPER_COMMON_H

#include <clang/AST/Mangle.h>

std::string getMangledName(clang::NamedDecl const *const nd);

#endif

#ifndef CGCOLLECTOR_HELPER_COMMOM_H
#define CGCOLLECTOR_HELPER_COMMON_H

#include <clang/AST/Mangle.h>
#include <string>
#include <vector>


/**
 * Returns mangled names for all named decls, including Ctor/Dtor.
 */
std::vector<std::string> getMangledName(clang::NamedDecl const *const nd);

#endif

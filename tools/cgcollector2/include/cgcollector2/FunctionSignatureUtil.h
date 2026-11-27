/**
 * File: FunctionSignatureUtil.h
 * License: Part of the MetaCG project. Licensed under BSD 3 clause license. See LICENSE.txt file at
 * https://github.com/tudasc/metacg/LICENSE.txt
 */

#ifndef CGCOLLECTOR2_FUNCTIONSIGNATUREUTIL_H
#define CGCOLLECTOR2_FUNCTIONSIGNATUREUTIL_H

#include "cgcollector2/SharedDefs.h"

#include <clang/AST/Decl.h>
#include <clang/AST/DeclCXX.h>
#include <clang/AST/Type.h>

#include <vector>

namespace cgcollector2 {

/**
 * Builds the signature of a function declaration.
 * For non-static member functions with an implicit object parameter, FunctionSignature::thisType is set.
 */
FunctionSignature getSignature(const clang::FunctionDecl* FD);

/**
 * Builds the signatures of all functions a call through a pointer of the given function type may reach.
 * @param calleeType the type of the function pointed to
 * @param memberOf the class of a pointer to member function, nullptr for an ordinary function pointer
 * @return For an ordinary function pointer, a single signature without implicit object parameter.
 *         For a pointer to member function of class C, one signature for C and one for each class C (transitively)
 *         inherits from non-virtually, as a pointer to a member of a base class implicitly converts to a pointer to
 *         a member of the derived class.
 */
std::vector<FunctionSignature> getCalleeSignatures(const clang::FunctionType* calleeType,
                                                   const clang::CXXRecordDecl* memberOf);

}  // namespace cgcollector2

#endif  // CGCOLLECTOR2_FUNCTIONSIGNATUREUTIL_H

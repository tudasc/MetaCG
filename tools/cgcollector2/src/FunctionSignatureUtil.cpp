/**
 * File: FunctionSignatureUtil.cpp
 * License: Part of the MetaCG project. Licensed under BSD 3 clause license. See LICENSE.txt file at
 * https://github.com/tudasc/metacg/LICENSE.txt
 */

#include "cgcollector2/FunctionSignatureUtil.h"

#include <clang/AST/ASTContext.h>
#include <clang/AST/ASTLambda.h>
#include <llvm/Config/llvm-config.h>
#include <llvm/Support/raw_ostream.h>

#include <algorithm>
#include <unordered_set>

namespace {

/// The name of the class, qualified and including template arguments, so it is unique across translation units
std::string getClassName(const clang::CXXRecordDecl* RD) {
  std::string name;
  llvm::raw_string_ostream os(name);
  RD->getNameForDiagnostic(os, RD->getASTContext().getPrintingPolicy(), /*Qualified=*/true);
  os.flush();
  return name;
}

/// The implicit object parameter as written for a member of class RD with the qualifiers of FPT, e.g. "const A &"
std::string getThisType(const clang::CXXRecordDecl* RD, const clang::FunctionProtoType* FPT) {
  std::string thisType;
  if (FPT) {
    thisType = FPT->getMethodQuals().getAsString();
    if (!thisType.empty()) {
      thisType += ' ';
    }
  }
  thisType += getClassName(RD);
  if (FPT) {
    switch (FPT->getRefQualifier()) {
      case clang::RQ_LValue:
        thisType += " &";
        break;
      case clang::RQ_RValue:
        thisType += " &&";
        break;
      case clang::RQ_None:
        break;
    }
  }
  return thisType;
}

/// Collects RD and all classes RD inherits from without a virtual base along the way.
/// Pointers to members of virtual bases can not be converted to pointers to members of the derived class.
void collectSelfAndNonVirtualBases(const clang::CXXRecordDecl* RD, std::vector<const clang::CXXRecordDecl*>& classes,
                                   std::unordered_set<const clang::CXXRecordDecl*>& seen) {
  RD = RD->getCanonicalDecl();
  if (!seen.insert(RD).second) {
    return;
  }
  classes.push_back(RD);
  const auto* def = RD->getDefinition();
  if (!def) {
    // A pointer to member of an incomplete class, we can not know its bases
    return;
  }
  for (const auto& base : def->bases()) {
    if (base.isVirtual()) {
      continue;
    }
    if (const auto* baseRD = base.getType()->getAsCXXRecordDecl()) {
      collectSelfAndNonVirtualBases(baseRD, classes, seen);
    }
  }
}

bool hasImplicitObjectParameter(const clang::CXXMethodDecl* MD) {
#if LLVM_VERSION_MAJOR >= 18
  return MD->isImplicitObjectMemberFunction();
#else
  return MD->isInstance();
#endif
}

/// A lambda without captures converts to an ordinary function pointer. Calling that pointer calls the static invoker
/// of the lambda, which forwards to the call operator. As the invoker is not part of the graph, the call operator has
/// to be reachable through ordinary function pointers.
bool isFunctionPointerConvertibleLambdaCallOperator(const clang::CXXMethodDecl* MD) {
  return clang::isLambdaCallOperator(MD) && MD->getParent()->getLambdaStaticInvoker() != nullptr;
}

}  // namespace

namespace cgcollector2 {

FunctionSignature getSignature(const clang::FunctionDecl* FD) {
  FunctionSignature signature;
  // according to the standard constructor and destructor have return type void
  if (!clang::isa<clang::CXXDestructorDecl>(FD) && !clang::isa<clang::CXXConstructorDecl>(FD)) {
    signature.retType = FD->getReturnType().getAsString();
  }
  std::transform(FD->parameters().begin(), FD->parameters().end(), std::back_inserter(signature.paramTypes),
                 [](const clang::ParmVarDecl* p) { return p->getType().getAsString(); });
  if (const auto* MD = clang::dyn_cast<clang::CXXMethodDecl>(FD);
      MD && hasImplicitObjectParameter(MD) && !isFunctionPointerConvertibleLambdaCallOperator(MD)) {
    signature.thisType = getThisType(MD->getParent(), MD->getType()->getAs<clang::FunctionProtoType>());
  }
  return signature;
}

std::vector<FunctionSignature> getCalleeSignatures(const clang::FunctionType* calleeType,
                                                   const clang::CXXRecordDecl* memberOf) {
  FunctionSignature signature;
  signature.retType = calleeType->getReturnType().getAsString();
  // A function without prototype (C only) has no known parameters
  const auto* FPT = clang::dyn_cast<clang::FunctionProtoType>(calleeType);
  if (FPT) {
    std::transform(FPT->getParamTypes().begin(), FPT->getParamTypes().end(), std::back_inserter(signature.paramTypes),
                   [](auto type) { return type.getAsString(); });
  }

  if (!memberOf) {
    return {signature};
  }

  std::vector<const clang::CXXRecordDecl*> classes;
  std::unordered_set<const clang::CXXRecordDecl*> seen;
  collectSelfAndNonVirtualBases(memberOf, classes, seen);

  std::vector<FunctionSignature> signatures;
  signatures.reserve(classes.size());
  for (const auto* RD : classes) {
    signature.thisType = getThisType(RD, FPT);
    signatures.push_back(signature);
  }
  return signatures;
}

}  // namespace cgcollector2

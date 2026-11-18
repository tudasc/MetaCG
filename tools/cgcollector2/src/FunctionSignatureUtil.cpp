/**
 * File: FunctionSignatureUtil.cpp
 * License: Part of the MetaCG project. Licensed under BSD 3 clause license. See LICENSE.txt file at
 * https://github.com/tudasc/metacg/LICENSE.txt
 */

#include "cgcollector2/FunctionSignatureUtil.h"

#include <clang/AST/ASTContext.h>
#include <clang/AST/ASTLambda.h>
#include <clang/AST/PrettyPrinter.h>
#include <clang/Basic/LangOptions.h>
#include <llvm/ADT/SmallString.h>
#include <llvm/Config/llvm-config.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/Support/Path.h>
#include <llvm/Support/raw_ostream.h>

#include <algorithm>
#include <unordered_set>

namespace {

/// Paths appear in the names of lambdas and other unnamed types ("(lambda at /path/file.h:1:2)").
/// They are spelled as included, so the same header reached via different include paths would give different names.
struct NormalizingPathCallbacks : clang::PrintingCallbacks {
  std::string remapPath(llvm::StringRef path) const override {
    llvm::SmallString<256> normalized(path);
    llvm::sys::fs::make_absolute(normalized);
    llvm::sys::path::remove_dots(normalized, /*remove_dot_dot=*/true);
    return std::string(normalized);
  }
};

/// The policy all types of a signature are printed with.
/// Signatures are compared as strings across translation units (when merging and when matching function pointers to
/// functions), so the printed form must not depend on how a type is spelled in a particular translation unit, nor
/// on its language options.
const clang::PrintingPolicy& getSignaturePrintingPolicy() {
  static const NormalizingPathCallbacks callbacks;
  static const clang::PrintingPolicy policy = [] {
    clang::LangOptions langOpts;
    langOpts.CPlusPlus = true;
    langOpts.Bool = true;
    clang::PrintingPolicy pp(langOpts);
    // Print canonical template arguments, otherwise e.g. explicit instantiations print their arguments as written
    // ("Foo<std::uint32_t>" instead of "Foo<unsigned int>")
#if LLVM_VERSION_MAJOR >= 21
    pp.PrintAsCanonical = true;
#else
    pp.PrintCanonicalTypes = true;
#endif
    pp.SuppressTagKeyword = true;
    pp.FullyQualifiedName = true;
    pp.Callbacks = &callbacks;
    return pp;
  }();
  return policy;
}

std::string typeToString(clang::QualType type) {
  return type.getCanonicalType().getAsString(getSignaturePrintingPolicy());
}

/// Top level qualifiers of parameters are not part of the function type, `void f(int)` and `void f(const int)` are the
/// same function, so they are dropped.
std::string paramTypeToString(clang::QualType type) {
  return type.getCanonicalType().getUnqualifiedType().getAsString(getSignaturePrintingPolicy());
}

/// The name of the class, qualified and including template arguments, so it is unique across translation units
std::string getClassName(const clang::CXXRecordDecl* RD) {
  std::string name;
  llvm::raw_string_ostream os(name);
  RD->getNameForDiagnostic(os, getSignaturePrintingPolicy(), /*Qualified=*/true);
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
    signature.retType = typeToString(FD->getReturnType());
  }
  std::transform(FD->parameters().begin(), FD->parameters().end(), std::back_inserter(signature.paramTypes),
                 [](const clang::ParmVarDecl* p) { return paramTypeToString(p->getType()); });
  if (const auto* MD = clang::dyn_cast<clang::CXXMethodDecl>(FD);
      MD && hasImplicitObjectParameter(MD) && !isFunctionPointerConvertibleLambdaCallOperator(MD)) {
    signature.thisType = getThisType(MD->getParent(), MD->getType().getCanonicalType()->getAs<clang::FunctionProtoType>());
  }
  return signature;
}

std::vector<FunctionSignature> getCalleeSignatures(const clang::FunctionType* calleeType,
                                                   const clang::CXXRecordDecl* memberOf) {
  FunctionSignature signature;
  signature.retType = typeToString(calleeType->getReturnType());
  // A function without prototype (C only) has no known parameters
  const auto* FPT = clang::dyn_cast<clang::FunctionProtoType>(calleeType);
  if (FPT) {
    std::transform(FPT->getParamTypes().begin(), FPT->getParamTypes().end(), std::back_inserter(signature.paramTypes),
                   [](clang::QualType type) { return paramTypeToString(type); });
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

/**
 * File: CallGraphNodeGenerator.cpp
 * License: Part of the MetaCG project. Licensed under BSD 3 clause license. See LICENSE.txt file at
 * https://github.com/tudasc/metacg/LICENSE.txt
 */

#include "CallGraphNodeGenerator.h"
#include <clang/AST/Decl.h>
#include <clang/AST/DeclBase.h>
#include <clang/AST/Expr.h>
#include <clang/Basic/IdentifierTable.h>
#include <clang/Basic/LLVM.h>
#include <llvm/Support/Casting.h>
#include <llvm/Support/Compiler.h>
#include <llvm/Support/GraphWriter.h>

#if LLVM_VERSION_MAJOR > 10

#include <clang/AST/ParentMapContext.h>

#endif

#include <cassert>

#include <clang/AST/Mangle.h>

#include "metadata/Internal/ASTNodeMetadata.h"
#include "metadata/Internal/AllAliasMetadata.h"
#include "metadata/Internal/FunctionSignatureMetadata.h"
#include "metadata/OverrideMD.h"

#include "LoggerUtil.h"

using namespace clang;

[[nodiscard]] inline bool starts_with(llvm::StringRef Str, llvm::StringRef Prefix) {
#if LLVM_VERSION_MAJOR < 17
  return Str.startswith(Prefix);
#else
  return Str.starts_with(Prefix);
#endif
}

inline bool ends_with(std::string const& value, std::string const& ending) {
  if (ending.size() > value.size()) {
    return false;
  }
  return std::equal(ending.rbegin(), ending.rend(), value.rbegin());
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

bool CallGraphNodeGenerator::TraverseFunctionDecl(clang::FunctionDecl* D) {
  SPDLOG_TRACE("{} {}", __FUNCTION__, (void*)D);
  // If function is ignored, we don't need to traverse its content
  if (!shouldIncludeFunction(D)) {
    return true;
  }
  auto oldFd = topLevelFD;
  topLevelFD = D;
  bool const retval = RecursiveASTVisitor::TraverseFunctionDecl(D);
  topLevelFD = oldFd;
  return retval;
}

bool CallGraphNodeGenerator::TraverseCXXMethodDecl(clang::CXXMethodDecl* MD) {
  SPDLOG_TRACE("{} {}", __FUNCTION__, (void*)MD);
  // If function is ignored, we don't need to traverse its content
  if (!shouldIncludeFunction(MD)) {
    return true;
  }
  auto oldFd = topLevelFD;
  topLevelFD = MD;
  bool const retval = RecursiveASTVisitor::TraverseCXXMethodDecl(MD);
  topLevelFD = oldFd;
  return retval;
}

bool CallGraphNodeGenerator::TraverseCXXConstructorDecl(clang::CXXConstructorDecl* D) {
  SPDLOG_TRACE("{} {}", __FUNCTION__, (void*)D);
  // If function is ignored, we don't need to traverse its content
  if (!shouldIncludeFunction(D)) {
    return true;
  }
  auto oldFd = topLevelFD;
  topLevelFD = D;
  bool const retval = RecursiveASTVisitor::TraverseCXXConstructorDecl(D);
  topLevelFD = oldFd;
  return retval;
}

bool CallGraphNodeGenerator::TraverseCXXDestructorDecl(clang::CXXDestructorDecl* D) {
  SPDLOG_TRACE("{} {}", __FUNCTION__, (void*)D);
  // If function is ignored, we don't need to traverse its content
  if (!shouldIncludeFunction(D)) {
    return true;
  }
  auto oldFd = topLevelFD;
  topLevelFD = D;
  bool const retval = RecursiveASTVisitor::TraverseCXXDestructorDecl(D);
  topLevelFD = oldFd;
  return retval;
}

bool CallGraphNodeGenerator::TraverseFunctionTemplateDecl(clang::FunctionTemplateDecl* D) {
  SPDLOG_TRACE("{} {}", __FUNCTION__, (void*)D);
  // In case the FunctionTemplateDecl is a CXXMemberFunctionTemplateDecl we expect them to be available

  for (const clang::FunctionDecl* const f : D->specializations()) {
    if (!shouldIncludeFunction(f)) {
      // ignoring
      continue;
    }
    addNode(f);
  }

  // We abort traversal of the template-class after traversing all specialisations
  // I don't think an uninstantiated template-class has any information left after this
  return true;  // high cuts: RecursiveASTVisitor::TraverseFunctionTemplateDecl(D);
}

bool CallGraphNodeGenerator::TraverseClassTemplateDecl(clang::ClassTemplateDecl* D) {
  SPDLOG_TRACE("{} {}", __FUNCTION__, (void*)D);
  // We do not traverse the uninstantiated class template description, but only their specialisations
  for (clang::ClassTemplateSpecializationDecl* const c : D->specializations()) {
    if (isa<clang::ClassTemplatePartialSpecializationDecl>(c)) {
      continue;  // Don't traverse partially specialized templates ?
    }
    if (traversedTemplates.find(c) == traversedTemplates.end()) {
      traversedTemplates.insert(c);  // Stop infinite recursion
      RecursiveASTVisitor::TraverseCXXRecordDecl(c);
    }
  }
  // We abort traversal of the template-class after traversing all specialisations
  // I don't think an uninstantiated template-class has any information left after this
  return true; // high cuts: RecursiveASTVisitor::TraverseClassTemplateDecl(D);
}

bool CallGraphNodeGenerator::shouldIncludeFunction(const Decl* D) {
  assert(D);
  // NOTE: It could make sense to check here that only FunctionDecls are included. Right now this function also returns
  // true for VarDecls/ParmVarDecls that are called because they contain a function pointer
  if (const auto* FD = dyn_cast<FunctionDecl>(D)) {
    // We skip function template definitions, as their semantics is
    // only determined when they are instantiated.
    if (FD->isDependentContext()) {
      // Node is context depended we dont capture;
      return false;
    }

    if (const auto* ND = dyn_cast<NamedDecl>(FD)) {
      if (!captureNewDelete &&
          (ends_with(ND->getDeclName().getAsString(), "new") || ends_with(ND->getDeclName().getAsString(), "new[]") ||
           ends_with(ND->getDeclName().getAsString(), "delete") ||
           ends_with(ND->getDeclName().getAsString(), "delete[]"))) {
        // The Node is a new or delete but we do not want to capture those
        return false;
      }
    }

    // if it is a constructor or destructor, and we don't capture those, return early
    if ((isa<CXXConstructorDecl>(D) || isa<CXXDestructorDecl>(D)) && !captureCtorsDtors) {
      // The Node is a Constructor or Destructor, but we do not want to capture
      return false;
    }

    // Check if we need to capture implicits
    if (FD->isImplicit() && !captureImplicits) {
      return false;
    }

    IdentifierInfo* II = FD->getIdentifier();
    // TODO not sure whether we want to include __inline marked functions
    if (II && starts_with(II->getName(), ("__inline"))) {
      return true;
    }
  } else {
    assert(false && "all nodes to check for graph inclusion should be function decls of some kind");
  }
  return true;
}

bool CallGraphNodeGenerator::VisitFunctionDecl(clang::FunctionDecl* FD) {
  SPDLOG_TRACE("{} {}", __FUNCTION__, (void*)FD);
  if (!shouldIncludeFunction(FD)) {
    return true;
  }

  if (!isa<NamedDecl>(FD)) {
    assert(false && "The current function decl is not named, this should not happen");
    return true;
  }

  addNode(FD);

  return true;
}

bool CallGraphNodeGenerator::VisitCallExpr(clang::CallExpr* E) {
  SPDLOG_TRACE("{} {}", __FUNCTION__, (void*)E);

  if (!topLevelFD) {
    SPDLOG_DEBUG("We are not inside a function, no call edge to be drawn for call {}", (void*)E);
    return true;
  }

  if (E->getDirectCallee() != nullptr) {
    SPDLOG_DEBUG("Handling direct call");
    const auto& directCallee = E->getDirectCallee();
    if (!shouldIncludeFunction(directCallee)) {
      return true;
    }
    if (!callgraph->hasNode(getMangledNames(directCallee).at(0))) {
      SPDLOG_DEBUG("The call from {} ({}) into {} ({}) was to a previously unobserved function, adding node on the fly",
                   topLevelFD->getNameAsString(), (void*)topLevelFD,
                   clang::cast<clang::NamedDecl>(directCallee)->getNameAsString(), (void*)directCallee);
      addNode(directCallee);
    }
    addEdge(E->getDirectCallee());
  } else if (E->getCalleeDecl() != nullptr) {
    SPDLOG_TRACE("There is no direct callee, but we can get some declaration this is going to be doable");
    auto nonDirectCallee = E->getCalleeDecl();
    // nonDirectCallee->dump();
    switch (level) {
      case No:
        addFunctionPointerAsEntity(nonDirectCallee);
        break;
      case All:
        addOverestimationData(nonDirectCallee);
        break;
      default:
        assert(false && "Alias Analysis level is corrupt");
        break;
    }
  } else if (E->getCallee()->getType()->isPointerType()) {
    // We got a callee which points to something"
    assert(E->getCallee()->getType()->isPointerType() && "Callee must point to a function we call");
    assert(E->getCallee()
               ->getType()
               ->getPointeeType()
               .getDesugaredType(topLevelFD->getASTContext())
               .IgnoreParens()
               ->isFunctionProtoType());
    auto prototype = clang::cast<clang::FunctionProtoType>(
        E->getCallee()->getType()->getPointeeType().getDesugaredType(topLevelFD->getASTContext()).IgnoreParens());
    addPointerMetadataFromPrototype(prototype);
  } else if (auto unresolvedLookupExpr = dyn_cast<UnresolvedLookupExpr>(E->getCallee())) {
    SPDLOG_WARN("The callee {} is currently unresolvable, we do not handle this",
                unresolvedLookupExpr->getName().getAsString());
    return true;
  } else if (isa<CXXPseudoDestructorExpr>(E->getCallee())) {
    // We are calling a destructor in a template, but instantiated the template with a primitive.
    // As primitives don't have a destructor this only looks like a call but is none
    return true;
  } else {
    SPDLOG_WARN("Wierd cases encountered!");
    if (E->getCallee()->getType()->isDependentType()) {
      assert(E->getCallee()->getType()->isDependentType() == E->getCallee()->isTypeDependent());
      SPDLOG_WARN(
          "When calling from {} ({}) to {}, the callee type is uninstantiated and is supposed to be resolved later."
          "We do not handle this.",
          topLevelFD->getNameAsString(), (void*)topLevelFD, (void*)E->getCallee());
      return true;
    }

    if (topLevelFD != nullptr) {
      SPDLOG_WARN("Totally unknown case for {} -> {}",
                  E->getSourceRange().printToString(topLevelFD->getASTContext().getSourceManager()),
                  E->getCallee()->getSourceRange().printToString(topLevelFD->getASTContext().getSourceManager()));
    } else {
      SPDLOG_WARN("Totally unknown case for {} -> {}", (void*)E, (void*)E->getCallee());
    }
    assert(false && "There are some other shenanigans going on with the callee");
  }
  return true;
}

bool CallGraphNodeGenerator::VisitCXXDestructorDecl(clang::CXXDestructorDecl* DD) {
  SPDLOG_TRACE("{} {}", __FUNCTION__, (void*)DD);
  if (!shouldIncludeFunction(DD) || !captureCtorsDtors) {
    return true;
  }

  const clang::CXXRecordDecl* ClassDecl = DD->getParent();
  if (ClassDecl == nullptr) {
    return true;
  }

  if (inferCtorsDtors) {
    // Check for base class destructors
    for (const auto& Base : ClassDecl->bases()) {
      const clang::CXXRecordDecl* BaseDecl = Base.getType()->getAsCXXRecordDecl();
      if (BaseDecl && BaseDecl->hasDefinition()) {
        if (clang::CXXDestructorDecl* BaseDestructor = BaseDecl->getDestructor()) {
          addEdge(BaseDestructor);
        }
      }
    }
    // Detect member variable destruction
    for (const clang::FieldDecl* Field : ClassDecl->fields()) {
      const clang::QualType FieldType = Field->getType();
      if (const clang::CXXRecordDecl* MemberClass = FieldType->getAsCXXRecordDecl()) {
        if (MemberClass->hasDefinition() && MemberClass->hasNonTrivialDestructor()) {
          if (clang::CXXDestructorDecl* MemberDestructor = MemberClass->getDestructor()) {
            addEdge(MemberDestructor);
          }
        }
      }
    }
  }
  return true;
}

bool CallGraphNodeGenerator::VisitCXXDeleteExpr(clang::CXXDeleteExpr* DE) {
  SPDLOG_TRACE("{} {}", __FUNCTION__, (void*)DE);
  assert(DE != nullptr);
  if (!captureCtorsDtors) {
    // We should ignore calls to a destructor via delete
    return true;
  }
  if (!topLevelFD) {
    SPDLOG_DEBUG("We are not inside a function, no call edge to be drawn for delete expression {}", (void*)DE);
    return true;
  }

  if (DE->getDestroyedType().isNull()) {
    assert(llvm::isa<DeclRefExpr>(*DE->child_begin()));
    assert(DE->getSourceRange().isValid());
    assert(topLevelFD != nullptr);
    SPDLOG_WARN("Found a destruction of an unknown type");
    // We do not know the type that is to be destroyed, we skip this
    return true;
  }

  // We should capture calls to a destructor via delete

  const auto* DECxxDecl = DE->getDestroyedType()->getAsCXXRecordDecl();
  if (!DECxxDecl) {
    // Deleted type is not a record decl (for example a primitive type)
    return true;
  }

  if (!shouldIncludeFunction(DECxxDecl->getDestructor())) {
    // The destructor itself is not captured, so no call will be mapped
    return true;
  }

  if (!callgraph->hasNode(getMangledNames(DECxxDecl->getDestructor()).at(0))) {
    SPDLOG_DEBUG("The call from {} ({}) into {} ({}) was to a previously unobserved function, adding node on the fly",
                 topLevelFD->getNameAsString(), (void*)topLevelFD,
                 clang::cast<clang::NamedDecl>(DECxxDecl->getDestructor())->getNameAsString(),
                 (void*)DECxxDecl->getDestructor());
    addNode(DECxxDecl->getDestructor());
  }

  addEdge(DECxxDecl->getDestructor());
  return true;
}

bool CallGraphNodeGenerator::VisitCXXConstructExpr(clang::CXXConstructExpr* CE) {
  SPDLOG_TRACE("{} {}", __FUNCTION__, (void*)CE);
  if (!topLevelFD) {
    SPDLOG_DEBUG("We are not inside a function, no call edge to be drawn for construct expression {}", (void*)CE);
    return true;
  }
  assert(CE->getConstructor() != nullptr);
  if (captureCtorsDtors) {
    // We should capture the call to a ConstructExpr
    if (!shouldIncludeFunction(CE->getConstructor())) {
      // The constructor itself is not captured, so no call will be mapped
      return true;
    }

    // We sometimes encounter a reference to a function, that has never been seen before
    // This can happen for compiler generated functions
    if (!callgraph->hasNode(getMangledNames(CE->getConstructor()).at(0))) {
      SPDLOG_DEBUG("The call from {} ({}) into {} ({}) was to a previously unobserved function, adding node on the fly",
                   topLevelFD->getNameAsString(), (void*)topLevelFD,
                   clang::cast<clang::NamedDecl>(CE->getConstructor())->getNameAsString(), (void*)CE->getConstructor());
      addNode(CE->getConstructor());
    }

    addEdge(CE->getConstructor());
  }
  return true;
}

// We need to visit the var decls and then filter for local CXXRecords as the underlying CXXConstructExpr does not allow
// us to access the storage kind
bool CallGraphNodeGenerator::VisitVarDecl(clang::VarDecl* VD) {
  SPDLOG_TRACE("{} {}", __FUNCTION__, (void*)VD);
  if (!inferCtorsDtors) {
    return true;
  }
  if (!VD->hasLocalStorage()) {
    return true;  // Only check local variables
  }
  if (const clang::CXXRecordDecl* RD = VD->getType()->getAsCXXRecordDecl()) {
    if (RD->hasDefinition()) {
      if (auto Dtor = RD->getDestructor()) {
        addEdge(Dtor);
      }
    }
  }
  return true;
}

bool CallGraphNodeGenerator::VisitCXXBindTemporaryExpr(clang::CXXBindTemporaryExpr* CXXBTE) {
  SPDLOG_TRACE("{} {}", __FUNCTION__, (void*)CXXBTE);
  if (!inferCtorsDtors) {
    return true;
  }
  assert(CXXBTE->getType()->getAsCXXRecordDecl() &&
         "Could not get the CXXRecordDeclaration from the temporary construct expression");
  assert(CXXBTE->getType()->getAsCXXRecordDecl()->getDestructor() &&
         "Could not get the Destructor from the temporarily constructed CXXRecord");
  const auto cxxbte = CXXBTE->getType()->getAsCXXRecordDecl()->getDestructor();
  addEdge(cxxbte);
  return true;
}

void CallGraphNodeGenerator::addNode(const clang::FunctionDecl* const D) {
  SPDLOG_TRACE("Adding node \"{}\" {} {}", D->getNameAsString(), (void*)topLevelFD,
               col2str(getMangledNames(D), "(", ",", ")"));
  for (auto& name : getMangledNames(D)) {
    // Add the function to the callgraph
    metacg::CgNode& node = callgraph->getOrInsertNode(name);
    // This is so we overwrite if we ever find the body of a function after a predeclare
    node.setHasBody(D->hasBody());
    if (auto cxxmethod = dyn_cast<CXXMethodDecl>(D); cxxmethod != nullptr && cxxmethod->isVirtual()) {
      node.getOrCreate<metacg::OverrideMD>();
    }
    node.getOrCreate<ASTNodeMetadata>().setFunctionDecl(D);
    if (!node.has<FunctionSignatureMetadata>()) {
      std::unique_ptr<FunctionSignatureMetadata> md = std::make_unique<FunctionSignatureMetadata>(!standalone);
       //according to the standard constructor and destructor have return type void
      md->ownSignature.possibleFuncNames.push_back(D->getNameAsString());
      clang::isa<CXXDestructorDecl>(D) || clang::isa<CXXConstructorDecl>(D)
          ? md->ownSignature.retType = ""
          : md->ownSignature.retType = D->getReturnType().getAsString();
      std::transform(D->parameters().begin(), D->parameters().end(), std::back_inserter(md->ownSignature.paramTypes),
                     [](ParmVarDecl* p) { return p->getType().getAsString(); });
      node.addMetaData(std::move(md));
    }
  }
}

void CallGraphNodeGenerator::addEdge(clang::Decl* Child) {
  assert(isa<clang::NamedDecl>(Child));
  assert(topLevelFD != nullptr);
  assert(Child != nullptr);
  SPDLOG_DEBUG("Calling from: {} {} into {} {} ", topLevelFD->getNameAsString(), (void*)topLevelFD,
               clang::cast<clang::NamedDecl>(Child)->getNameAsString(), (void*)Child);
  for (auto& parentName : getMangledNames(topLevelFD)) {
    for (auto& childName : getMangledNames(clang::cast<clang::NamedDecl>(Child))) {
      assert(callgraph->hasNode(parentName));
      assert(callgraph->hasNode(childName));
      // If parent calls child multiple times inside its body this will be true the second time
      //Fixme: This is probably not a good idea but works for now
      if (callgraph->existsAnyEdge(parentName, childName)) {
        // This is to silence warnings about existing edges
        continue;
      }
      callgraph->addEdge(parentName, childName);
    }
  }
}

void CallGraphNodeGenerator::addFunctionPointerAsEntity(clang::Decl* D) {
  assert(clang::isa<clang::NamedDecl>(D));
  auto FP = clang::cast<clang::NamedDecl>(D);
  assert(!callgraph->hasNode(FP->getDeclName().getAsString()) && "non direct calls should not have a node allready");
  assert(getMangledNames(FP).size() == 1 && "Mangled names for non direct calls should be unique");
  callgraph->insert(getMangledNames(FP).at(0));
  addEdge(D);
  assert(!clang::isa<clang::FunctionDecl>(D) &&
         "If this callee decl is a function we would have a direct callee available");
}

void CallGraphNodeGenerator::addOverestimationData(clang::Decl* nonDirectCallee) {
  assert(nonDirectCallee && "nonDirectCallee callee should not be nullptr");
  assert(clang::isa<clang::DeclaratorDecl>(nonDirectCallee) &&
         "Field and Var decls share this base class, if this failed, we have an unkown callee type");

  clang::QualType calleeType = getFinalPointee(clang::cast<clang::DeclaratorDecl>(nonDirectCallee)->getType());
  if (calleeType.isNull()) {
    // if the pointee type is null, it is unknown what type we are calling into
    // this should only happen if we call into a templateType via implicit interface.
    assert(clang::isa<clang::TemplateTypeParmType>(calleeType));
    return;
  }
  auto prototype = clang::cast<clang::FunctionProtoType>(calleeType);
  addPointerMetadataFromPrototype(prototype);
}

void CallGraphNodeGenerator::addPointerMetadataFromPrototype(const clang::FunctionProtoType* protoType) {
  for (const auto& possibleName : getMangledNames(topLevelFD)) {
    assert(callgraph->getNodes(possibleName).size()==1 && "We currently only handle name-unique nodes");
    auto anyNodeID =callgraph->getNodes(possibleName)[0];
    auto& md = callgraph->getNode(anyNodeID)->getOrCreate<AllAliasMetadata>(!standalone);
    FunctionSignature functionSignature;
    functionSignature.retType = protoType->getReturnType().getAsString();
    std::transform(protoType->getParamTypes().begin(), protoType->getParamTypes().end(),
                   std::back_inserter(functionSignature.paramTypes), [](auto type) { return type.getAsString(); });
    functionSignature.possibleFuncNames.emplace_back("");
    md.mightCall.push_back(functionSignature);
  }
}

clang::QualType CallGraphNodeGenerator::getFinalPointee(clang::QualType pointerType) {
  clang::QualType finalType = pointerType.IgnoreParens().getCanonicalType().IgnoreParens();
  while (!finalType->getPointeeType().isNull()) {
    finalType = finalType->getPointeeType().IgnoreParens().getCanonicalType().IgnoreParens();
  }
  return finalType;
}

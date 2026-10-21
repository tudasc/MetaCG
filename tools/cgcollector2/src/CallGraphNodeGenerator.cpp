/**
 * File: CallGraphNodeGenerator.cpp
 * License: Part of the MetaCG project. Licensed under BSD 3 clause license. See LICENSE.txt file at
 * https://github.com/tudasc/metacg/LICENSE.txt
 */

#include "cgcollector2/CallGraphNodeGenerator.h"
#include "cgcollector2/metadata/internal/ASTNodeMetadata.h"
#include "cgcollector2/metadata/external/AllAliasMetadata.h"
#include "cgcollector2/metadata/external/FunctionSignatureMetadata.h"
#include "metacg/LoggerUtil.h"
#include "metacg/metadata/OverrideMD.h"

#include <clang/AST/Decl.h>
#include <clang/AST/DeclBase.h>
#include <clang/AST/Expr.h>
#include <clang/AST/Mangle.h>
#include <clang/Basic/IdentifierTable.h>
#include <clang/Basic/LLVM.h>
#include <clang/Sema/Template.h>
#include <llvm/Support/Casting.h>
#include <llvm/Support/Compiler.h>
#include <llvm/Support/GraphWriter.h>
#if LLVM_VERSION_MAJOR > 10
#include <clang/AST/ParentMapContext.h>
#endif
#if LLVM_VERSION_MAJOR >= 17
#include <clang/Sema/EnterExpressionEvaluationContext.h>
#endif

#include <cassert>

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
    llvm::errs() << "NamedDecl was nullptr\n";
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
  metacg::MCGLogger::instance().getConsole()->trace("{} {}", __FUNCTION__, (void*)D);
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
  metacg::MCGLogger::instance().getConsole()->trace("{} {}", __FUNCTION__, (void*)MD);
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
  metacg::MCGLogger::instance().getConsole()->trace("{} {}", __FUNCTION__, (void*)D);
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
  metacg::MCGLogger::instance().getConsole()->trace("{} {}", __FUNCTION__, (void*)D);
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

  metacg::MCGLogger::instance().getConsole()->trace("{} {}", __FUNCTION__, (void*)D);
  // We do not traverse the uninstantiated function template description, but only their specializations, so calls
  // inside them get attributed to the instantiated function. Uninstantiated templates therefore do not show up at all
  for (clang::FunctionDecl* const f : D->specializations()) {
    if (f->getTemplateSpecializationKind() == clang::TSK_ExplicitSpecialization) {
      continue;  // Explicit specializations are regular decls in their decl context and are traversed from there
    }
    if (!shouldIncludeFunction(f)) {
      continue;
    }
    if (traversedTemplates.find(f) == traversedTemplates.end()) {
      traversedTemplates.insert(f);  // Stop infinite recursion
      // Dispatches to TraverseFunctionDecl / TraverseCXXMethodDecl / ..., which add the node and set topLevelFD
      if (!TraverseDecl(f)) {
        return false;
      }
    }
  }

  // We abort traversal of the template-function after traversing all specializations
  // I don't think an uninstantiated template-function has any information left after this
  return true;  // high cuts: RecursiveASTVisitor::TraverseFunctionTemplateDecl(D);
}

bool CallGraphNodeGenerator::TraverseClassTemplateDecl(clang::ClassTemplateDecl* D) {
  metacg::MCGLogger::instance().getConsole()->trace("{} {}", __FUNCTION__, (void*)D);
  // We do not traverse the uninstantiated class template description, but only their specializations
  for (clang::ClassTemplateSpecializationDecl* const c : D->specializations()) {
    if (isa<clang::ClassTemplatePartialSpecializationDecl>(c)) {
      continue;  // Don't traverse partially specialized templates ?
    }
    if (traversedTemplates.find(c) == traversedTemplates.end()) {
      traversedTemplates.insert(c);  // Stop infinite recursion
      RecursiveASTVisitor::TraverseCXXRecordDecl(c);
    }
  }
  // We abort traversal of the template-class after traversing all specializations
  // I don't think an uninstantiated template-class has any information left after this
  return true;  // high cuts: RecursiveASTVisitor::TraverseClassTemplateDecl(D);
}

bool CallGraphNodeGenerator::TraverseStmt(clang::Stmt* S, DataRecursionQueue* Queue) {
  // If we find any expression, that is inside a function but not fully instantiated, we need to resolve it with Sema
  if (auto* E = dyn_cast_or_null<Expr>(S); E && topLevelFD && E->isInstantiationDependent()) {
    // As we traverse top-down, E is the root of this uninstantiated part.
    // Resolve it against the template arguments of topLevelFD.
    return traverseUninstantiatedExpr(E);
  }
  return RecursiveASTVisitor::TraverseStmt(S, Queue);
}

#if LLVM_VERSION_MAJOR >= 15
// Simplified version of the private Sema::addInstantiatedParametersToScope: Maps the parameters of the pattern to the
// ones of the instantiation, so references to them can be substituted.
// Returns false if the parameters of the instantiation do not line up with the ones of the pattern.
static bool addInstantiatedParametersToScope(Sema& sema, FunctionDecl* function, const FunctionDecl* pattern,
                                             const MultiLevelTemplateArgumentList& templateArgs,
                                             LocalInstantiationScope& scope) {
  const unsigned numPatternParams = pattern->getNumParams();
  const unsigned numParams = function->getNumParams();
  unsigned paramIdx = 0;

  for (unsigned patternParamIdx = 0; patternParamIdx < numPatternParams; ++patternParamIdx) {
    const ParmVarDecl* patternParam = pattern->getParamDecl(patternParamIdx);
    if (!patternParam->isParameterPack()) {
      if (paramIdx >= numParams) {
        return false;
      }
      scope.InstantiatedLocal(patternParam, function->getParamDecl(paramIdx++));
      continue;
    }

    // Ask Sema how many arguments the pack expands to for these template arguments, so multiple packs work as well
    const auto packSize = sema.getNumArgumentsInExpansion(patternParam->getType(), templateArgs);
    if (!packSize || numParams - paramIdx < *packSize) {
      return false;
    }
    scope.MakeInstantiatedLocalArgPack(patternParam);
    for (const unsigned packEnd = paramIdx + *packSize; paramIdx < packEnd; ++paramIdx) {
      scope.InstantiatedLocalPackArg(patternParam, function->getParamDecl(paramIdx));
    }
  }
  return paramIdx == numParams;
}
#endif

bool CallGraphNodeGenerator::traverseUninstantiatedExpr(clang::Expr* E) {
#if LLVM_VERSION_MAJOR >= 15

  const FunctionDecl* pattern = topLevelFD->getTemplateInstantiationPattern(false);
  if (!sema) {
    SPDLOG_DEBUG("Sema is unavailable for this run");
    return true;
  }

  if (!pattern) {
    SPDLOG_DEBUG("Can not find original template description for {}. Left uninstantiated in expression {}, skipping it",
                 topLevelFD->getNameAsString(), (void*)E);
    return true;
  }

  // Collect the template initialization args
#if LLVM_VERSION_MAJOR >= 18
  const MultiLevelTemplateArgumentList templateArgs = sema->getTemplateInstantiationArgs(
      topLevelFD,  // D: decl we compute template args for
      nullptr,     // DC: use topLevelFD's own decl context
      false,       // Final: keep the type sugar of the substituted arguments
      {},          // Innermost: no explicit innermost args, take them from topLevelFD's specialization info
      true,        // RelativeToPrimary: compute the arguments relative to the primary template
      nullptr,     // Pattern: let clang look up the template pattern of topLevelFD itself
      true);       // ForConstraintInstantiation: also walk through generic lambdas and enclosing class templates
#elif LLVM_VERSION_MAJOR >= 16
  const MultiLevelTemplateArgumentList templateArgs = sema->getTemplateInstantiationArgs(
      topLevelFD,  // D: decl we compute template args for
      false,       // Final: keep the type sugar of the substituted arguments
      nullptr,     // Innermost: no explicit innermost args, take them from topLevelFD's specialization info
      true,        // RelativeToPrimary: compute the arguments relative to the primary template
      nullptr,     // Pattern: let clang look up the template pattern of topLevelFD itself
      true);       // ForConstraintInstantiation: also walk through generic lambdas and enclosing class templates
#else
  const MultiLevelTemplateArgumentList templateArgs = sema->getTemplateInstantiationArgs(
      topLevelFD,  // D: decl we compute template args for
      nullptr,     // Innermost: no explicit innermost args, take them from topLevelFD's specialization info
      true,        // RelativeToPrimary: compute the arguments relative to the primary template
      nullptr);    // Pattern: let clang look up the template pattern of topLevelFD itself
#endif

  ExprResult substituted = ExprError();
  // AFAICT one communicates with SEMA via RAII constructor-destructor states
  {
    //Save the old Sema context from the current topLevel FD
    //This will be restored to the whatever scope we need once the stack object is destroyed
    Sema::ContextRAII savedContext(*sema, topLevelFD);

    //Set up the SEMA RAII constructs for the substituion
    LocalInstantiationScope scope(*sema);
    EnterExpressionEvaluationContext unevaluated(*sema, Sema::ExpressionEvaluationContext::Unevaluated);
    Sema::InstantiatingTemplate inst(*sema, E->getBeginLoc(), topLevelFD);
    Sema::SFINAETrap trap(*sema);

    if (!inst.isInvalid() && addInstantiatedParametersToScope(*sema, topLevelFD, pattern, templateArgs, scope)) {
      substituted = sema->SubstExpr(E, templateArgs);
    }

    if (substituted.isInvalid() || trap.hasErrorOccurred() || !substituted.get() ||
        substituted.get()->isInstantiationDependent()) {
      // The expression is ill-formed for these template arguments (e.g. an unsatisfied requirement), so it can not call
      // anything
      SPDLOG_DEBUG("Uninstantiated expression {} inside {} ({}) is not resolvable for its template arguments", (void*)E,
                   topLevelFD->getNameAsString(), (void*)topLevelFD);
      return true;
    }
  }

  SPDLOG_DEBUG("Resolved uninstantiated expression {} inside {} ({}) to {}", (void*)E, topLevelFD->getNameAsString(),
               (void*)topLevelFD, (void*)substituted.get());
  //Continue traversing the fully substituted statement as normal
  return RecursiveASTVisitor::TraverseStmt(substituted.get());
#else
  SPDLOG_DEBUG("Can not resolve uninstantiated expression {} inside {} ({}) with this LLVM version, skipping it",
               (void*)E, topLevelFD->getNameAsString(), (void*)topLevelFD);
  return true;
#endif
}

bool CallGraphNodeGenerator::shouldIncludeFunction(const Decl* D) const {
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
  metacg::MCGLogger::instance().getConsole()->trace("{} {}", __FUNCTION__, (void*)FD);
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
  metacg::MCGLogger::instance().getConsole()->trace("{} {}", __FUNCTION__, (void*)E);

  if (!topLevelFD) {
    metacg::MCGLogger::instance().getConsole()->debug(
        "We are not inside a function, no call edge to be drawn for call {}", (void*)E);
    return true;
  }

  if (E->isInstantiationDependent()) {
    // Usually resolved in TraverseStmt; if a substituted expression is still partly dependent
    SPDLOG_DEBUG("Skipping uninstantiated call {} inside {} ({})", (void*)E, topLevelFD->getNameAsString(),
                 (void*)topLevelFD);
    return true;
  }

  if (E->getDirectCallee() != nullptr) {
    metacg::MCGLogger::instance().getConsole()->debug("Handling direct call");
    const auto& directCallee = E->getDirectCallee();
    if (!shouldIncludeFunction(directCallee)) {
      return true;
    }
    addEdge(E->getDirectCallee());
  } else if (E->getCalleeDecl() != nullptr) {
    metacg::MCGLogger::instance().getConsole()->trace(
        "There is no direct callee, but we can get some declaration this is going to be doable");
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
    metacg::MCGLogger::logWarn("The callee {} is currently unresolvable, we do not handle this",
                               unresolvedLookupExpr->getName().getAsString());
    return true;
  } else if (isa<CXXPseudoDestructorExpr>(E->getCallee())) {
    // We are calling a destructor in a template, but instantiated the template with a primitive.
    // As primitives don't have a destructor this only looks like a call but is none
    return true;
  } else {
    metacg::MCGLogger::logWarn("Found unsupported call expression construct");
    if (E->getCallee()->getType()->isDependentType()) {
      assert(E->getCallee()->getType()->isDependentType() == E->getCallee()->isTypeDependent());
      metacg::MCGLogger::logWarn(
          "When calling from {} ({}) to {}, the callee type is uninstantiated and is supposed to be resolved later."
          "We should have handled this and this should not happen anymore.",
          topLevelFD->getNameAsString(), (void*)topLevelFD, (void*)E->getCallee());
      return true;
    }
    if (topLevelFD != nullptr) {
      metacg::MCGLogger::logWarn(
          "Totally unknown case for {} -> {}",
          E->getSourceRange().printToString(topLevelFD->getASTContext().getSourceManager()),
          E->getCallee()->getSourceRange().printToString(topLevelFD->getASTContext().getSourceManager()));
    } else {
      metacg::MCGLogger::logWarn("Totally unknown case for {} -> {}", (void*)E, (void*)E->getCallee());
    }
    assert(false && "There are some other shenanigans going on with the callee");
  }
  return true;
}

bool CallGraphNodeGenerator::VisitCXXDestructorDecl(clang::CXXDestructorDecl* DD) {
  metacg::MCGLogger::instance().getConsole()->trace("{} {}", __FUNCTION__, (void*)DD);
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
  metacg::MCGLogger::instance().getConsole()->trace("{} {}", __FUNCTION__, (void*)DE);
  assert(DE != nullptr);
  if (!captureCtorsDtors) {
    // We should ignore calls to a destructor via delete
    return true;
  }
  if (!topLevelFD) {
    metacg::MCGLogger::instance().getConsole()->debug(
        "We are not inside a function, no call edge to be drawn for delete expression {}", (void*)DE);
    return true;
  }

  if (DE->getDestroyedType().isNull()) {
    assert(llvm::isa<DeclRefExpr>(*DE->child_begin()));
    assert(DE->getSourceRange().isValid());
    assert(topLevelFD != nullptr);
    metacg::MCGLogger::logWarn("Found a destruction of an unknown type");
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

  addEdge(DECxxDecl->getDestructor());
  return true;
}

bool CallGraphNodeGenerator::VisitCXXConstructExpr(clang::CXXConstructExpr* CE) {
  metacg::MCGLogger::instance().getConsole()->trace("{} {}", __FUNCTION__, (void*)CE);
  if (!topLevelFD) {
    metacg::MCGLogger::instance().getConsole()->debug(
        "We are not inside a function, no call edge to be drawn for construct expression {}", (void*)CE);
    return true;
  }
  assert(CE->getConstructor() != nullptr);
  if (captureCtorsDtors) {
    // We should capture the call to a ConstructExpr
    if (!shouldIncludeFunction(CE->getConstructor())) {
      // The constructor itself is not captured, so no call will be mapped
      return true;
    }

    if (CE->isElidable()) {
      // We are interested in the constructor, but the construction is elided, so no call to the constructor will happen
      return true;
    }

    addEdge(CE->getConstructor());
  }
  return true;
}

// We need to visit the var decls and then filter for local CXXRecords as the underlying CXXConstructExpr does not allow
// us to access the storage kind
bool CallGraphNodeGenerator::VisitVarDecl(clang::VarDecl* VD) {
  metacg::MCGLogger::instance().getConsole()->trace("{} {}", __FUNCTION__, (void*)VD);
  if (!inferCtorsDtors) {
    return true;
  }

  if (!VD->hasLocalStorage()) {
    return true;  // Only check local variables
  }

  if (const clang::CXXRecordDecl* RD = VD->getType()->getAsCXXRecordDecl()) {
    // TODO: Edges to trivial destructors are order dependent.
    //  Sema declares an implicit destructor lazily, so getDestructor() only returns it
    //  if something else in the TU needed it (e.g. a temporary of the same type).
    //  `C c;` alone yields no destructor edge, `C c; C{}.f();` does.
    //  Either skip trivial destructors RD->hasNonTrivialDestructor()
    //  or force the declaration (Sema::LookupDestructor) to make this consistent.
    //  The ground truth of the cxxRecordCalls tests :
    //  0006, 0008, 0014, 0016, 0018 and allCtorDtor 0010
    //  rely on the current wrong behavior.
    if (RD->hasDefinition()) {
      if (auto Dtor = RD->getDestructor()) {
        addEdge(Dtor);
      }
    }
  }
  return true;
}

bool CallGraphNodeGenerator::VisitCXXBindTemporaryExpr(clang::CXXBindTemporaryExpr* CXXBTE) {
  metacg::MCGLogger::instance().getConsole()->trace("{} {}", __FUNCTION__, (void*)CXXBTE);
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
  metacg::MCGLogger::instance().getConsole()->trace("Adding node \"{}\" {} {}", D->getNameAsString(), (void*)topLevelFD,
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
      // according to the standard constructor and destructor have return type void
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
  metacg::MCGLogger::instance().getConsole()->debug(
      "Calling from: {} {} into {} {} ", topLevelFD->getNameAsString(), (void*)topLevelFD,
      clang::cast<clang::NamedDecl>(Child)->getNameAsString(), (void*)Child);
  for (auto& parentName : getMangledNames(topLevelFD)) {
    for (auto& childName : getMangledNames(clang::cast<clang::NamedDecl>(Child))) {
      assert(callgraph->hasNode(parentName));
      if (!callgraph->hasNode(childName)) {
        assert(isa<FunctionDecl>(Child));
        // This can happen if we have an immediately invoked lambda,
        //  or infer a call to a destructor via a param variable declaration not referring to a definition
        //  FIXME: and other cases we do not understand.
        metacg::MCGLogger::instance().getConsole()->debug(
            "The call from {} ({}) into {} ({}) was to a previously unobserved function, adding node on the fly",
            parentName, (void*)topLevelFD, clang::cast<clang::NamedDecl>(Child)->getNameAsString(), (void*)Child);
        addNode(cast<FunctionDecl>(Child));
      }
      assert(callgraph->hasNode(childName));
      // If parent calls child multiple times inside its body this will be true the second time
      // Fixme: This is probably not a good idea but works for now
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
    assert(callgraph->getAllNodesWithName(possibleName).size() == 1 && "We currently only handle name-unique nodes");
    auto anyNodeID = callgraph->getAllNodesWithName(possibleName)[0];
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

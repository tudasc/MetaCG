/**
 * File: OverridesPlugin.cpp
 * License: Part of the MetaCG project. Licensed under BSD 3 clause license. See LICENSE.txt file at
 * https://github.com/tudasc/metacg/LICENSE.txt
 */

#include "OverridesPlugin.h"

#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Frontend/FrontendPluginRegistry.h"
#include <cassert>
#include <clang/AST/APValue.h>
#include <clang/AST/ASTContext.h>
#include <clang/AST/Attrs.inc>
#include <clang/AST/ComparisonCategories.h>
#include <clang/AST/Decl.h>
#include <clang/AST/DeclCXX.h>
#include <clang/AST/Expr.h>
#include <clang/AST/Mangle.h>
#include <clang/AST/Type.h>
#include <clang/Basic/AttributeCommonInfo.h>
#include <clang/Basic/DiagnosticOptions.h>
#include <clang/Basic/SourceLocation.h>
#include <clang/CodeGen/CodeGenAction.h>
#include <llvm/ADT/APSInt.h>
#include <llvm/ADT/SmallVector.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/InlineAsm.h>
#include <llvm/IR/Module.h>
#include <llvm/Support/raw_ostream.h>
#include <map>
#include <memory>
#include <string>
#include <vector>

using namespace cage::overrides;
namespace {

using MethodDecl = const clang::CXXMethodDecl*;
using OverrideDS = std::map<MethodDecl, std::vector<MethodDecl>>;

class OverridesVisitor final : public clang::RecursiveASTVisitor<OverridesVisitor> {
 public:
  bool VisitCXXMethodDecl(clang::CXXMethodDecl* MD) {
    if (!MD->isThisDeclarationADefinition())
      return true;

    clang::Expr* argsArray[1];
    clang::ASTContext* Context = &MD->getASTContext();
    argsArray[0] = getConstantStringExpr(Context);

    for (const auto& base : MD->overridden_methods()) {
      std::string mangledName = getMangledName(base);

      // Annotate
      std::string annotationStr = mangledName;
      clang::AnnotateAttr* attr = clang::AnnotateAttr::CreateImplicit(*Context, annotationStr, argsArray, 1);

      MD->addAttr(attr);
    }

    return true;
  }

 private:
  clang::ConstantExpr* getConstantStringExpr(clang::ASTContext* Context) {
    const llvm::StringRef overridePluginString = "overridePlugin";

    uint64_t stringLength = overridePluginString.size();
    clang::QualType charArrayTy = Context->getStringLiteralArrayType(Context->CharTy, stringLength);

    clang::StringLiteral* StrLit =
        clang::StringLiteral::Create(*Context, overridePluginString, clang::StringLiteralKind::Ordinary,
                                     /*Pascal=*/false, charArrayTy, clang::SourceLocation());

    clang::APValue Value(clang::APValue::LValueBase(StrLit), clang::CharUnits::Zero(), clang::APValue::NoLValuePath());

    clang::ConstantExpr* CE = clang::ConstantExpr::Create(*Context, StrLit, Value);

    return CE;
  }
  std::string getMangledName(const clang::NamedDecl* name) {
    std::unique_ptr<clang::MangleContext> MC(name->getASTContext().createMangleContext());

    std::string mangledName;
    llvm::raw_string_ostream os(mangledName);

    MC->mangleName(name, os);
    os.flush();

    return mangledName;
  }
};

}  // namespace

namespace cage::overrides {

/*
 * HandleTranslationUnit does eagerly emit global function defintions. E.g. inline definition A::foo override {} is eagerly 
 * emitted before HandleTranslationUnit runs and therefore cannot be annotated.
 */
bool OverridesPluginConsumer::HandleTopLevelDecl(clang::DeclGroupRef declGroup) {
  OverridesVisitor visitor;
  for (clang::Decl* decl : declGroup) {
    visitor.TraverseDecl(decl);
  }
  return true;
}

std::unique_ptr<clang::ASTConsumer> OverridesPluginAction::CreateASTConsumer(
    [[maybe_unused]] clang::CompilerInstance& compiler, [[maybe_unused]] llvm::StringRef inFile) {
  return std::make_unique<OverridesPluginConsumer>();
}

bool OverridesPluginAction::ParseArgs(const clang::CompilerInstance& compiler, const std::vector<std::string>& args) {
  // Parse plugin arguments here if needed
  return true;
}

}  // namespace cage::overrides

static clang::FrontendPluginRegistry::Add<cage::overrides::OverridesPluginAction> X(
    "cage-overrides-plugin", "MetaCG CaGe overrides clang plugin skeleton");

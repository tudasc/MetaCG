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
#include <clang/AST/ASTContext.h>
#include <clang/AST/Attrs.inc>
#include <clang/AST/DeclCXX.h>
#include <clang/Basic/AttributeCommonInfo.h>
#include <clang/Basic/DiagnosticOptions.h>
#include <clang/Basic/SourceLocation.h>
#include <clang/CodeGen/CodeGenAction.h>
#include <llvm/ADT/SmallVector.h>
#include <llvm/IR/Module.h>
#include <llvm/Support/raw_ostream.h>
#include <map>
#include <vector>

using namespace cage::overrides;
namespace {

using MethodDecl = const clang::CXXMethodDecl*;
using OverrideDS = std::map<MethodDecl, std::vector<MethodDecl>>;

class OverridesVisitor final : public clang::RecursiveASTVisitor<OverridesVisitor> {
 clang::ASTContext& Context;
 OverrideDS& Overrides;

public:
  explicit OverridesVisitor(clang::ASTContext& context, OverrideDS& overrides)
      : Context(context), Overrides(overrides) {}

  bool VisitCXXMethodDecl(clang::CXXMethodDecl* MD) {
    
    if (!MD->isVirtual()) {
      return true;
    }

    auto annotation = clang::AnnotateAttr::CreateImplicit(Context, "HelloWorld", nullptr, 0, clang::SourceRange());
    auto* canonicalMD = MD->getCanonicalDecl();
    canonicalMD->addAttr(annotation);
    assert(canonicalMD->getAttr<clang::AnnotateAttr>());

    for (const auto& base : MD->getCanonicalDecl()->overridden_methods()) {
      llvm::outs() << "LOG: " << base->getCanonicalDecl()->getQualifiedNameAsString() << "\n";
    }
    return true;
  }

private:
};

}  // namespace

namespace cage::overrides {

void OverridesPluginConsumer::HandleTranslationUnit(clang::ASTContext& context) {
  // Create the override map locally in the consumer
  OverrideDS overrides;
  OverridesVisitor visitor(context, overrides);
  visitor.TraverseDecl(context.getTranslationUnitDecl());
  context.getTranslationUnitDecl()->addAttr(clang::AnnotateAttr::CreateImplicit(context, "HelloWorld", nullptr, 0, clang::SourceRange()));
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

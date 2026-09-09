/**
* File: OverridesPlugin.h
* License: Part of the MetaCG project. Licensed under BSD 3 clause license. See LICENSE.txt file at
* https://github.com/tudasc/metacg/LICENSE.txt
*/

#ifndef METACG_CAGE_OVERRIDES_PLUGIN_H
#define METACG_CAGE_OVERRIDES_PLUGIN_H

#include "clang/AST/ASTConsumer.h"
#include "clang/Frontend/FrontendAction.h"

#include <clang/AST/DeclGroup.h>
#include <clang/Frontend/CompilerInstance.h>
#include <llvm/Support/raw_ostream.h>

namespace cage::overrides {

class OverridesPluginConsumer final : public clang::ASTConsumer {
 public:
  explicit OverridesPluginConsumer() {}
  //void HandleTranslationUnit(clang::ASTContext& context) override;
  bool HandleTopLevelDecl(clang::DeclGroupRef declGroup);
};

class OverridesPluginAction final : public clang::PluginASTAction {
 protected:
  std::unique_ptr<clang::ASTConsumer> CreateASTConsumer(clang::CompilerInstance& compiler,
                                                        llvm::StringRef inFile) override;

  bool ParseArgs(const clang::CompilerInstance& compiler, const std::vector<std::string>& args) override;

  ActionType getActionType() override {
    return AddBeforeMainAction;
  }
};

}  // namespace cage::overrides

#endif  // METACG_CAGE_OVERRIDES_PLUGIN_H

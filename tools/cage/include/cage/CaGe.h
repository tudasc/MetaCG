/**
 * File: Plugin.h
 * License: Part of the MetaCG project. Licensed under BSD 3 clause license. See LICENSE.txt file at
 * https://github.com/tudasc/metacg/LICENSE.txt
 */
#ifndef METACG_CAGE_PLUGIN_H
#define METACG_CAGE_PLUGIN_H

#include "llvm/Pass.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"

namespace cage {

struct CaGe : llvm::PassInfoMixin<CaGe> {
  llvm::PreservedAnalyses run(llvm::Module& M, llvm::ModuleAnalysisManager& MA);

  static llvm::StringRef name() { return "CaGe"; }
};
}  // namespace cage

#endif  // METACG_CAGE_PLUGIN_H

/**
 * File: CallGraphGenerator.h
 * License: Part of the MetaCG project. Licensed under BSD 3 clause license. See LICENSE.txt file at
 * https://github.com/tudasc/metacg/LICENSE.txt
 */
#include "../interface/CaGePlugin.h"

#include "io/MCGWriter.h"

#include "llvm/IR/Module.h"
#include "llvm/IR/PassManager.h"

namespace cage {

enum PTAType { No, BySignature };

class Generator {
 public:
  explicit Generator(PTAType ptaType) : ptaType(ptaType) {};

  void addPlugin(std::unique_ptr<Plugin> consumer) { plugins.push_back(std::move(consumer)); }

  bool run(llvm::Module& M, llvm::ModuleAnalysisManager* MA);

 private:
  PTAType ptaType;
  std::vector<std::unique_ptr<Plugin>> plugins;
};

}  // namespace cage

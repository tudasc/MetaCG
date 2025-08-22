/**
 * File: CallGraphGenerator.h
 * License: Part of the MetaCG project. Licensed under BSD 3 clause license. See LICENSE.txt file at
 * https://github.com/tudasc/metacg/LICENSE.txt
 */
#include "cage/generator/CallGraphConsumer.h"

#include "io/MCGWriter.h"

#include "llvm/IR/Module.h"
#include "llvm/IR/PassManager.h"

namespace cage {

class Generator {
 public:
  Generator() = default;

  void addConsumer(std::unique_ptr<CallGraphConsumer> consumer) { consumers.push_back(std::move(consumer)); }

  bool run(llvm::Module& M, llvm::ModuleAnalysisManager* MA);

 private:
  std::vector<std::unique_ptr<CallGraphConsumer>> consumers;
};

}  // namespace cage

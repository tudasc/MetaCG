/**
 * File: Plugin.cpp
 * License: Part of the MetaCG project. Licensed under BSD 3 clause license. See LICENSE.txt file at
 * https://github.com/tudasc/metacg/LICENSE.txt
 */
#include "cage/Plugin.h"

#include "llvm/IR/LegacyPassManager.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"

#include "cage/generator/CallgraphGenerator.h"
#include "cage/generator/FileExporter.h"

using namespace llvm;

using namespace llvm::cl;

static OptionCategory cageOpts("CaGe");

static cl::opt<bool> cageVerbose("cage-verbose", cl::desc("Print debugging output"), cl::init(false));

static opt<cage::PTAType> pta(
    "pta", desc("Points-to analysis of indirect calls:"),
    values(clEnumValN(cage::PTAType::No, "no", "Ignore indirect calls"),
           clEnumValN(
               cage::PTAType::BySignature, "signature",
               "Treat all available valid function signatures for a given function pointer as potential call target ")),
    cat(cageOpts), init(cage::PTAType::No));


namespace cage {
PreservedAnalyses CaGe::run(Module& M, ModuleAnalysisManager& MA) {

  if (cageVerbose) {
    outs() << "Running in verbose mode\n";
  }

  Generator gen(pta);
  gen.addConsumer(std::make_unique<FileExporter>());

  if (!gen.run(M, &MA))
    return PreservedAnalyses::all();

  return PreservedAnalyses::none();
}
}  // namespace cage

llvm::PassPluginLibraryInfo getPluginInfo() {
  return {LLVM_PLUGIN_API_VERSION, "CaGe", "0.2", [](PassBuilder& PB) {
            // allow registration via optlevel (non-lto)
#if LLVM_VERSION_MAJOR >= 20
            PB.registerOptimizerLastEPCallback([](ModulePassManager& PM, OptimizationLevel, ThinOrFullLTOPhase) {
#else
            PB.registerOptimizerLastEPCallback([](ModulePassManager& PM, OptimizationLevel) {
#endif
              outs() << "Registering CaGe to run during opt\n";
              PM.addPass(cage::CaGe());
            });

            // registering via optlevel during lto appears to still be broken
            PB.registerFullLinkTimeOptimizationLastEPCallback([](ModulePassManager& PM, OptimizationLevel o) {
              outs() << "Registering CaGe to run during full LTO\n";
              PM.addPass(cage::CaGe());
            });

            // allow registration via pipeline parser
            PB.registerPipelineParsingCallback(
                [](StringRef Name, ModulePassManager& MPM, ArrayRef<llvm::PassBuilder::PipelineElement>) {
                  if (Name == "CaGe") {
                    outs() << "Registering CaGe to run as pipeline described\n";
                    MPM.addPass(cage::CaGe());
                    return true;
                  } else {
                    outs() << "Did not register CaGe\n";
                  }
                  return false;
                });
          }};
}


extern "C" LLVM_ATTRIBUTE_WEAK ::llvm::PassPluginLibraryInfo llvmGetPassPluginInfo() {
#ifndef NDEBUG
  outs() << "Loading debug version of CaGe-Plugin\n";
#else
  outs() << "Loading release version of CaGe-Plugin\n";
#endif
  return getPluginInfo();
}
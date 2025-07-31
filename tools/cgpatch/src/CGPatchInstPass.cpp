/**
 * File: cgpatch-pass.cpp
 * License: Part of the MetaCG project. Licensed under BSD 3 clause license. See LICENSE.txt file at
 * https://github.com/tudasc/metacg/LICENSE.txt
 */

#include "nlohmann/json.hpp"
#include "llvm/Demangle/Demangle.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Module.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/raw_ostream.h"
#include <fstream>
#include <iostream>
#include <llvm/ADT/StringRef.h>
#include <string>

#include "CallAnalysis.h"
#include "io/VersionTwoMCGWriter.h"

using namespace llvm;
using namespace cgpatch;

// option parsing
static cl::opt<bool> instrumentCtorsDtors("instrument-ctors-dtors", cl::desc("Instrument constructors and destructors"),
                                          cl::init(false));

static cl::opt<bool> verbose("cgpatch-verbose", cl::desc("Print debugging output"), cl::init(false));

namespace {

void insertMetaCGCall(Instruction& ins, Function& f, Value* calledOperand, Function* runtimeFunction);

// Instrumentation
void instrumentIndirectCalls(Module& M) {
  ItaniumPartialDemangler demangler;

  // Counter variables
  int indirectCallCount{0};
  int ctorDtorCallCount{0};
  // Create function declaration
  LLVMContext& Context = M.getContext();
  auto functionType =
      FunctionType::get(Type::getVoidTy(Context), {PointerType::get(Context, 0), PointerType::get(Context, 0)}, false);
  Function* runtimeFunction = cast<Function>(M.getOrInsertFunction("__metacg_indirect_call", functionType).getCallee());

  for (Function& F : M) {
    if (verbose) {
      outs() << "Traversing function: " << F.getName() << "\n";
    }
    for (BasicBlock& B : F)
      for (Instruction& Ins : B) {
        // Check if Ins is a call instruction
        auto* CB = dyn_cast<CallBase>(&Ins);
        auto CT = detectCallType(CB);

        if (verbose) {  // Only print if verbose is true
          printCallTypeInfo(CT, CB);
        }
        if (CT == CallType::Unknown)
          continue;

        if (isDirect(CT)) {  // Direct call
          // Instrument constructors & destructors if option is enabled
          if (instrumentCtorsDtors) {
            auto calledFunction = CB->getCalledFunction();
            // Setup demangler's internal state to work on the called function name
            demangler.partialDemangle(calledFunction->getName().str().c_str());
            if (demangler.isCtorOrDtor()) {  // constructor call
              insertMetaCGCall(Ins, F, CB->getCalledOperand(), runtimeFunction);
              ctorDtorCallCount++;
            }
          }
        } else {  // indirect call
          if (CT == CallType::Virtual)
            continue;

          insertMetaCGCall(Ins, F, CB->getCalledOperand(), runtimeFunction);
          indirectCallCount++;
        }
      }
  }
  if (verbose) {
    llvm::outs() << "[Info] Instrumented " << (indirectCallCount + ctorDtorCallCount) << " function calls in "
                 << M.getName().str() << ":\n"
                 << "\t" << indirectCallCount << ": "
                 << "Indirect functions.\n"
                 << "\t" << ctorDtorCallCount << ": "
                 << "constructors and destructors."
                 << "\n";
  }
}

// Insert call to __metacg_indirect_call before the current instruction
void insertMetaCGCall(Instruction& ins, Function& f, Value* calledOperand, Function* runtimeFunction) {
  IRBuilder<> Builder(&ins);
  auto* strArg = Builder.CreateGlobalString(f.getName().str());
  Builder.CreateCall(runtimeFunction, {strArg, calledOperand});
}

struct CGPatchInst : PassInfoMixin<CGPatchInst> {
  PreservedAnalyses run(Module& M, ModuleAnalysisManager&) {
    instrumentIndirectCalls(M);
    return PreservedAnalyses::all();
  }
  // We also need to be able to instrument optnone annotated functions
  static bool isRequired() { return true; }
};
}  // namespace

// Registration of the new pass
llvm::PassPluginLibraryInfo getCGPatchInstPluginInfo() {
  return {LLVM_PLUGIN_API_VERSION, "cgpatch-inst", LLVM_VERSION_STRING, [](PassBuilder& PB) {
            PB.registerPipelineStartEPCallback(
                [](ModulePassManager& MPM, OptimizationLevel l) { MPM.addPass(CGPatchInst()); });
          }};
}

extern "C" LLVM_ATTRIBUTE_WEAK::llvm::PassPluginLibraryInfo llvmGetPassPluginInfo() {
  return getCGPatchInstPluginInfo();
}

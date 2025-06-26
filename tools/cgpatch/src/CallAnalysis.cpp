/**
 * File: CallAnalysis.cpp
 * License: Part of the MetaCG project. Licensed under BSD 3 clause license. See LICENSE.txt file at
 * https://github.com/tudasc/metacg/LICENSE.txt
 */

#include "CallAnalysis.h"

#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/InstrTypes.h"
#include "llvm/IR/IntrinsicInst.h"
#include "llvm/IR/Module.h"

using namespace llvm;

namespace cgpatch {

CallType detectCallType(CallBase* Call) {
  // We detect the following pattern for calling virtual functions
  //   %1 = load ptr, ptr %obj -> Loading object
  //   %2 = load ptr, ptr %1   -> Loading vtable ptr from object
  //   %3 = getelementptr ptr, ptr %2, i32 index -> Getting function address location from vtable
  //   %4 = load ptr, ptr %3   -> Loading function address
  //   call void %4(ptr %obj)  -> Calling virtual function

  // Check if call is null
  if (!Call) {
    return Unknown;
  }

  // Check for direct callee
  if (Call->getCalledFunction()) {
    return Direct;
  }

  Value* FuncPtr = Call->getCalledOperand();

  // Check for alias
  if (auto* Alias = dyn_cast<GlobalAlias>(FuncPtr)) {
    if (auto* AF = dyn_cast<Function>(Alias->getAliasee())) {
      return DirectAlias;
    }
  }

  LoadInst* FuncLoad = dyn_cast<LoadInst>(FuncPtr);
  if (!FuncLoad) {
    return Indirect;
  }
  Value* FuncSource = FuncLoad->getPointerOperand();
  GetElementPtrInst* GEP = dyn_cast<GetElementPtrInst>(FuncSource);
  if (!GEP) {
    return Indirect;
  }
  Value* VTablePtr = GEP->getPointerOperand();

  // Get type test metadata. If we don't have this, it could just be a regular indirect call mimicking the same
  // pattern.
  Value* TypeTestMD = getTypeTestMetadata(VTablePtr);
  if (!TypeTestMD) {
    return Indirect;
  }

  LoadInst* VTableLoad = dyn_cast<LoadInst>(VTablePtr);
  if (!VTableLoad) {
    return Indirect;
  }

  Value* ObjectPtr = VTableLoad->getPointerOperand();
  Value* Idx = GEP->getOperand(1);

  // Finally, check if first argument is 'this'
  if (Call->arg_size() > 0 && Call->getArgOperand(0) == ObjectPtr) {
    return Virtual;
  }

  // Unknown because the combination of an apparent VTable load, but not passing 'this' is very strange.
  return Unknown;
}

Value* getTypeTestMetadata(Value* V) {
  for (auto* U : V->users()) {
    if (auto* Intr = dyn_cast<IntrinsicInst>(U)) {
      auto ID = Intr->getIntrinsicID();
      if (ID == Intrinsic::public_type_test || ID == Intrinsic::type_test) {
        return Intr;
      }
    }
  }
  return nullptr;
}

void printCallTypeInfo(CallType CT, llvm::CallBase* CB) {
  if (!CB) {
    return;
  }

  switch (CT) {
    case Virtual:
      llvm::outs() << "Virtual call identified: " << *CB << "\n";
      break;
    case Indirect:
      llvm::outs() << "Call is other indirect call: " << *CB << "\n";
      break;
    case Direct:
      llvm::outs() << "Call is direct call: " << *CB << "\n";
      break;
    case DirectAlias:
      llvm::outs() << "Call is direct call via a function alias: " << *CB << "\n";
      break;
    case Unknown:
      llvm::outs() << "Call type is unknown: " << *CB << "\n";
      break;
    default:
      assert(false);
  }
}

}  // namespace cgpatch

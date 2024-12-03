/**
 * File: CallAnalysis.h
 * License: Part of the MetaCG project. Licensed under BSD 3 clause license. See LICENSE.txt file at
 * https://github.com/tudasc/metacg/LICENSE.txt
 *
 * Created by Sebastian on 3/7/25.
 */

#ifndef META_CG_CG_PATCH_CALLANALYSIS_H
#define META_CG_CG_PATCH_CALLANALYSIS_H

namespace llvm {
class Value;
class CallBase;
}  // namespace llvm

namespace cgpatch {

enum CallType { Direct, DirectAlias, Virtual, Indirect, Unknown };

inline bool isDirect(CallType CT) { return CT == Direct || CT == DirectAlias; }

CallType detectCallType(llvm::CallBase* Call);

llvm::Value* getTypeTestMetadata(llvm::Value* V);

void printCallTypeInfo(CallType CT, llvm::CallBase* CB);
}  // namespace cgpatch

#endif  // META_CG_CG_PATCH_CALLANALYSIS_H

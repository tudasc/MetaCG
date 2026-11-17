/**
 * File: NumOperationsCollector.cpp
 * License: Part of the MetaCG project. Licensed under BSD 3 clause license. See LICENSE.txt file at
 * https://github.com/tudasc/metacg/LICENSE.txt
 */

#include "cgcollector2/collector/NumOperationsCollector.h"
#include <clang/AST/RecursiveASTVisitor.h>

namespace {
// Operations Counting
/**
 * This class is kinda experimental and does not handle all Code Constructs as can be seen in teh function VisitStmt
 * The handling of constructors and dynamic memory allocations is also still very basic
 * Generally speaking this class assumes a very naive code generation, without optimisations and uses the following
 * assumptions:
 * - Accesses/operations with doubles are twice as expensive as floats
 * - Global variables and complex parameters require memory accesses, while simle types as ints are in registers.
 * - If a variable is accessed multiple times in a function with a declref expression it is counted multiple times
 */
class NumOperationsVisitor : public clang::RecursiveASTVisitor<NumOperationsVisitor> {
 public:
  int numIntOps = 0;
  int numFloatOps = 0;
  int numControlFlowOps = 0;
  int numMemoryAccesses = 0;

  bool shouldVisitTemplateInstantiations() const { return true; }

  /**
   *  This function is only a fallback and contains a comment with all unimplemented language constructs
   * @return
   */
  bool VisitStmt(clang::Stmt*) {
    /*switch (s->getStmtClass()) {
      case clang::Stmt::NoStmtClass:
        llvm_unreachable("NoStmtClass should never be used");
        break;

        //      case clang::Stmt::GCCAsmStmtClass: // asm
        //      case clang::Stmt::MSAsmStmtClass: // asm
        //      case clang::Stmt::OMPAtomicDirectiveClass:
        //      case clang::Stmt::OMPBarrierDirectiveClass:
        //      case clang::Stmt::OMPCancelDirectiveClass:
        //      case clang::Stmt::OMPCancellationPointDirectiveClass:
        //      case clang::Stmt::OMPCriticalDirectiveClass:
        //      case clang::Stmt::OMPFlushDirectiveClass:
        //      case clang::Stmt::OMPDistributeDirectiveClass:
        //      case clang::Stmt::OMPDistributeParallelForDirectiveClass:
        //      case clang::Stmt::OMPDistributeParallelForSimdDirectiveClass:
        //      case clang::Stmt::OMPDistributeSimdDirectiveClass:
        //      case clang::Stmt::OMPForDirectiveClass:
        //      case clang::Stmt::OMPForSimdDirectiveClass:
        //      case clang::Stmt::OMPMasterTaskLoopDirectiveClass:
        //      case clang::Stmt::OMPMasterTaskLoopSimdDirectiveClass:
        //      case clang::Stmt::OMPParallelForDirectiveClass:
        //      case clang::Stmt::OMPParallelForSimdDirectiveClass:
        //      case clang::Stmt::OMPParallelMasterTaskLoopDirectiveClass:
        //      case clang::Stmt::OMPParallelMasterTaskLoopSimdDirectiveClass:
        //      case clang::Stmt::OMPSimdDirectiveClass:
        //      case clang::Stmt::OMPTargetParallelForSimdDirectiveClass:
        //      case clang::Stmt::OMPTargetSimdDirectiveClass:
        //      case clang::Stmt::OMPTargetTeamsDistributeDirectiveClass:
        //      case clang::Stmt::OMPTargetTeamsDistributeParallelForDirectiveClass:
        //      case clang::Stmt::OMPTargetTeamsDistributeParallelForSimdDirectiveClass:
        //      case clang::Stmt::OMPTargetTeamsDistributeSimdDirectiveClass:
        //      case clang::Stmt::OMPTaskLoopDirectiveClass:
        //      case clang::Stmt::OMPTaskLoopSimdDirectiveClass:
        //      case clang::Stmt::OMPTeamsDistributeDirectiveClass:
        //      case clang::Stmt::OMPTeamsDistributeParallelForDirectiveClass:
        //      case clang::Stmt::OMPTeamsDistributeParallelForSimdDirectiveClass:
        //      case clang::Stmt::OMPTeamsDistributeSimdDirectiveClass:
        //      case clang::Stmt::OMPMasterDirectiveClass:
        //      case clang::Stmt::OMPOrderedDirectiveClass:
        //      case clang::Stmt::OMPParallelDirectiveClass:
        //      case clang::Stmt::OMPParallelMasterDirectiveClass:
        //      case clang::Stmt::OMPParallelSectionsDirectiveClass:
        //      case clang::Stmt::OMPSectionDirectiveClass:
        //      case clang::Stmt::OMPSectionsDirectiveClass:
        //      case clang::Stmt::OMPSingleDirectiveClass:
        //      case clang::Stmt::OMPTargetDataDirectiveClass:
        //      case clang::Stmt::OMPTargetDirectiveClass:
        //      case clang::Stmt::OMPTargetEnterDataDirectiveClass:
        //      case clang::Stmt::OMPTargetExitDataDirectiveClass:
        //      case clang::Stmt::OMPTargetParallelDirectiveClass:
        //      case clang::Stmt::OMPTargetParallelForDirectiveClass:
        //      case clang::Stmt::OMPTargetTeamsDirectiveClass:
        //      case clang::Stmt::OMPTargetUpdateDirectiveClass:
        //      case clang::Stmt::OMPTaskDirectiveClass:
        //      case clang::Stmt::OMPTaskgroupDirectiveClass:
        //      case clang::Stmt::OMPTaskwaitDirectiveClass:
        //      case clang::Stmt::OMPTaskyieldDirectiveClass:
        //      case clang::Stmt::OMPTeamsDirectiveClass:
        //      case clang::Stmt::ObjCAtCatchStmtClass:
        //      case clang::Stmt::ObjCAtFinallyStmtClass:
        //      case clang::Stmt::ObjCAtSynchronizedStmtClass:
        //      case clang::Stmt::ObjCAtThrowStmtClass:
        //      case clang::Stmt::ObjCAtTryStmtClass:
        //      case clang::Stmt::ObjCAutoreleasePoolStmtClass:
        //      case clang::Stmt::ObjCForCollectionStmtClass:
        //      case clang::Stmt::OMPArraySectionExprClass:
        //      case clang::Stmt::ObjCArrayLiteralClass:
        //      case clang::Stmt::ObjCAvailabilityCheckExprClass:
        //      case clang::Stmt::ObjCBoolLiteralExprClass:
        //      case clang::Stmt::ObjCBoxedExprClass:
        //      case clang::Stmt::ObjCDictionaryLiteralClass:
        //      case clang::Stmt::ObjCEncodeExprClass:
        //      case clang::Stmt::ObjCIndirectCopyRestoreExprClass:
        //      case clang::Stmt::ObjCIsaExprClass:
        //      case clang::Stmt::ObjCIvarRefExprClass:
        //      case clang::Stmt::ObjCMessageExprClass:
        //      case clang::Stmt::ObjCPropertyRefExprClass:
        //      case clang::Stmt::ObjCProtocolExprClass:
        //      case clang::Stmt::ObjCSelectorExprClass:
        //      case clang::Stmt::ObjCStringLiteralClass:
        //      case clang::Stmt::ObjCSubscriptRefExprClass:
        //      case clang::Stmt::MSDependentExistsStmtClass:
        //      case clang::Stmt::CUDAKernelCallExprClass:
        //      case clang::Stmt::ObjCBridgedCastExprClass:
        //      case clang::Stmt::MSPropertyRefExprClass:
        //      case clang::Stmt::MSPropertySubscriptExprClass:

        //      case clang::Stmt::CXXRewrittenBinaryOperatorClass: // c++ 20
        // case clang::Stmt::CXXTypeidExprClass:  // c++
        // case clang::Stmt::CXXUuidofExprClass: // ms

        //      case clang::Stmt::SEHExceptStmtClass: Windows exceptions
        //      case clang::Stmt::SEHFinallyStmtClass:
        //      case clang::Stmt::SEHLeaveStmtClass:
        //      case clang::Stmt::SEHTryStmtClass:

        //      case clang::Stmt::CoawaitExprClass: C++ coroutines
        //      case clang::Stmt::CoyieldExprClass:
        //      case clang::Stmt::DependentCoawaitExprClass:
        //      case clang::Stmt::CoreturnStmtClass:
        //      case clang::Stmt::CoroutineBodyStmtClass:

      case clang::Stmt::ArrayInitIndexExprClass:
      case clang::Stmt::
          ArrayInitLoopExprClass:  // https://clang.llvm.org/doxygen/classclang_1_1ArrayInitLoopExpr.html#details
      case clang::Stmt::ArrayTypeTraitExprClass:  // Used for some compiler builtin functions
      case clang::Stmt::AsTypeExprClass:          // Compiler builtin for casts
      case clang::Stmt::AtomicExprClass:          // Atomic builtins
        //      case clang::Stmt::ConceptSpecializationExprClass:  // c++ concepts
      case clang::Stmt::ConvertVectorExprClass:  // __builtin_convertvector
        // case clang::Stmt::LambdaExprClass:                 // c++ This is basically the same as a fucntion
      case clang::Stmt::InitListExprClass:  // Initilist
      case clang::Stmt::DesignatedInitExprClass:
      case clang::Stmt::DesignatedInitUpdateExprClass:
      case clang::Stmt::ImplicitValueInitExprClass:
      case clang::Stmt::NoInitExprClass:
      case clang::Stmt::ExpressionTraitExprClass:   // c++ builtin
      case clang::Stmt::ExtVectorElementExprClass:  // Language extension?
      case clang::Stmt::GNUNullExprClass:           // builtin extension
      case clang::Stmt::OpaqueValueExprClass:       //?
      case clang::Stmt::PseudoObjectExprClass:      //?
      case clang::Stmt::RequiresExprClass:          // c++ required
      case clang::Stmt::ShuffleVectorExprClass:     // builtin extension
      case clang::Stmt::TypeTraitExprClass:         // c++ bultin
      case clang::Stmt::VAArgExprClass:             // builtin
      case clang::Stmt::CapturedStmtClass:          // openmp
      case clang::Stmt::BuiltinBitCastExprClass:    // c++ builtin
        break;                                      // Unimplemented
    }
    // s->dump();
    return true;*/
    return true;
  }

  // Access
  bool VisitDeclRefExpr(clang::DeclRefExpr* ref) {
    // We assume local variables are in registers
    // TODO, make this more precise
    if (const auto vardecl = llvm::dyn_cast<clang::VarDecl>(ref->getDecl())) {
      if (vardecl->hasGlobalStorage()) {
        addToIntOrFloatOp(ref->getType(), true);
      } else if (llvm::isa<clang::ParmVarDecl>(vardecl)) {
        const auto type = vardecl->getType();
        if (!(type->isBooleanType() || type->isIntegerType() || type->isFloatingType() || type->isPointerType() ||
              type->isArrayType())) {
          addToIntOrFloatOp(type, true);
        }
      }
    }
    // References to functions should be handled during assembling
    return true;
  }

  bool VisitArraySubscriptExpr(clang::ArraySubscriptExpr* e) {
    // One calculation for the offset and one memory access
    numIntOps += 1;
    addToIntOrFloatOp(e->getType(), true);
    return true;
  }

  bool VisitMemberExpr(clang::MemberExpr* e) {
    addToIntOrFloatOp(e->getType(), true);
    return true;
  }

  bool VisitDeclStmt(clang::DeclStmt* d) {
    // TODO more cases
    if (d->isSingleDecl()) {
      if (auto vardecl = llvm::dyn_cast<clang::VarDecl>(d->getSingleDecl())) {
        if (vardecl->hasInit()) {
          addToIntOrFloatOp(vardecl->getType());
        }
      }
    }

    return true;
  }

  // Casts

  // https://gcc.gnu.org/onlinedocs/gcc/Compound-Literals.html
  //  bool VisitCompoundLiteralExpr(clang::CompoundLiteralExpr *) {
  //   This is basically cast and should be free
  //    return true;
  //  }

  bool VisitImplicitCastExpr(clang::ImplicitCastExpr* e) {
    if (e->isPartOfExplicitCast() || e->getCastKind() == clang::CastKind::CK_LValueToRValue ||
        e->getCastKind() == clang::CastKind::CK_LValueToRValueBitCast) {
      return true;
    }
    handleCast(e);
    return true;
  }

  bool VisitExplicitCastExpr(clang::ExplicitCastExpr* e) {
    handleCast(e);
    return true;
  }

  // Calculations

  bool VisitCompoundAssignOperator(clang::CompoundAssignOperator* op) {
    auto subExpr = llvm::dyn_cast<clang::DeclRefExpr>(op->getLHS());
    if (subExpr != nullptr) {
      VisitDeclRefExpr(subExpr);
    }
    addToIntOrFloatOp(op->getComputationResultType());
    return true;
  }

  bool VisitBinaryOperator(clang::BinaryOperator* op) {
    addToIntOrFloatOp(op->getType());
    return true;
  }

  bool VisitUnaryOperator(clang::UnaryOperator* op) {
    addToIntOrFloatOp(op->getType());
    if (op->isIncrementDecrementOp()) {
      auto subExpr = llvm::dyn_cast<clang::DeclRefExpr>(op->getSubExpr());
      if (subExpr != nullptr) {
        VisitDeclRefExpr(subExpr);
      }
    }
    return true;
  }

  // Conditional Operations

  //  bool VisitSwitchStmt(clang::SwitchStmt *s) {
  //    return true;
  //  }
  bool VisitSwitchCase(clang::SwitchCase*) {
    // This is quit much guessing, but lets  assume that we need one comparison and one control flow for each case
    numControlFlowOps += 1;
    numIntOps += 1;
    return true;
  }

  bool VisitIfStmt(clang::IfStmt* s) {
    numControlFlowOps += 1;
    if (s->hasElseStorage()) {
      numControlFlowOps += 1;
    }
    if (s->getCond()) {
      addToIntOrFloatOp(s->getCond()->getType());
    }
    return true;
  }

  // ? operator
  bool VisitAbstractConditionalOperator(clang::AbstractConditionalOperator* s) {
    numControlFlowOps += 1;
    addToIntOrFloatOp(s->getCond()->getType());
    return true;
  }

  // __builtin_choose_expr
  bool VisitChooseExpr(clang::ChooseExpr* e) {
    numControlFlowOps += 1;
    addToIntOrFloatOp(e->getCond()->getType());
    return true;
  }

  //  // _Generic, evalauted at compile time
  //  bool VisitGenericSelectionExpr(clang::GenericSelectionExpr *e) {
  //    return true;
  //  }

  bool VisitDoStmt(clang::DoStmt* s) {
    numControlFlowOps += 2;  // 1 for the conditional jump, 1 for the jump back
    if (s->getCond()) {
      addToIntOrFloatOp(s->getCond()->getType());
    }
    return true;
  }

  bool VisitWhileStmt(clang::WhileStmt* s) {
    numControlFlowOps += 2;  // 1 for the conditional jump, 1 for the jump back
    if (s->getCond()) {
      addToIntOrFloatOp(s->getCond()->getType());
    }
    return true;
  }

  bool VisitForStmt(clang::ForStmt* s) {
    numControlFlowOps += 2;  // 1 for the conditional jump, 1 for the jump back
    if (s->getCond()) {
      addToIntOrFloatOp(s->getCond()->getType());
    }
    return true;
  }

  bool VisitCXXForRangeStmtClass(clang::CXXForRangeStmt* s) {
    numControlFlowOps += 2;  // 1 for the conditional jump, 1 for the jump back
    if (s->getCond()) {
      addToIntOrFloatOp(s->getCond()->getType());
    }
    return true;
  }

  // Unconditional Control Flow
  bool VisitCallExpr(clang::CallExpr*) {
    // TODO maybe handle inline functions
    numControlFlowOps += 1;
    return true;
  }

  bool VisitCXXMemberCallExpr(clang::CXXMemberCallExpr*) {
    numMemoryAccesses += 1;
    numControlFlowOps += 1;
    return true;
  }

  bool VisitCXXOperatorCallExpr(clang::CXXOperatorCallExpr*) {
    numControlFlowOps += 1;
    return true;
  }

  bool VisitCXXInheritedCtorInitExpr(clang::CXXInheritedCtorInitExpr*) {
    numControlFlowOps += 2;
    return true;
  }

  bool VisitCXXCatchStmt(clang::CXXCatchStmt*) {
    // Two function calls
    numIntOps += 2;
    return true;
  }

  bool VisitCXXThrowExpr(clang::CXXThrowExpr* e) {
    numControlFlowOps += 2;
    if (e->getSubExpr()) {
      addToIntOrFloatOp(e->getSubExpr()->getType(), true);
    }
    return true;
  }

  // TODO Handle constructors
  bool VisitCXXNewExpr(clang::CXXNewExpr*) {
    numControlFlowOps += 1;
    return true;
  }

  bool VisitCXXDeleteExpr(clang::CXXDeleteExpr*) {
    // We likely have a destructor call too, which does not appear in the ast
    numControlFlowOps += 2;
    return true;
  }

  bool VisitCXXConstructExpr(clang::CXXConstructExpr*) {
    numControlFlowOps += 2;
    return true;
  }

  bool VisitReturnStmt(clang::ReturnStmt*) {
    numControlFlowOps += 1;
    return true;
  }

  bool VisitBreakStmt(clang::BreakStmt*) {
    numControlFlowOps += 1;
    return true;
  }

  bool VisitContinueStmt(clang::ContinueStmt*) {
    numControlFlowOps += 1;
    return true;
  }

  bool VisitGotoStmt(clang::GotoStmt*) {
    numControlFlowOps += 1;
    return true;
  }

  // GOTO to the 'address' of a label
  bool VisitIndirectGotoStmt(clang::IndirectGotoStmt*) {
    numControlFlowOps += 1;
    return true;
  }

 private:
  void handleCast(clang::CastExpr* e) {
    assert(e);
    if (e->getType()->isFloatingType() || e->getSubExpr()->getType()->isFloatingType()) {
      numFloatOps += 1;
    }
  }

  void addToIntOrFloatOp(clang::QualType t, bool mem_access = false) {
    if (t->isIntegerType() || t->isBooleanType() || t->isPointerType() || t->isArrayType() || t->isEnumeralType() ||
        t->isFunctionType() || t->isReferenceType()) {
      if (mem_access) {
        numMemoryAccesses += 1;
      } else {
        numIntOps += 1;
      }
    } else if (t->isFloatingType()) {
      if (mem_access) {
        numMemoryAccesses += 1;
      } else {
        if (const auto ftype = llvm::dyn_cast<clang::BuiltinType>(t->getUnqualifiedDesugaredType())) {
          const auto kind = ftype->getKind();
          if (!(kind == clang::BuiltinType::Kind::Half || kind == clang::BuiltinType::Kind::Float ||
                kind == clang::BuiltinType::Kind::Float16)) {
            numFloatOps += 2;
          } else {
            numFloatOps += 1;
          }
        }
      }
    } else if (const auto record = llvm::dyn_cast<clang::RecordType>(t->getUnqualifiedDesugaredType())) {
      if (const auto decl = record->getDecl()) {
        for (const auto f : decl->fields()) {
          addToIntOrFloatOp(f->getType(), mem_access);
        }
      }
    } else if (t->isMemberFunctionPointerType()) {
      // Skip, we already handled the member expression
    } else if (const auto tdef = llvm::dyn_cast<clang::TypedefType>(t.getUnqualifiedType())) {
      addToIntOrFloatOp(tdef->desugar(), mem_access);
    } else {
      const auto builtin = llvm::dyn_cast<clang::BuiltinType>(t->getUnqualifiedDesugaredType());
      if (builtin && builtin->getKind() == clang::BuiltinType::Kind::BoundMember) {
        return;
      }
      // llvm::errs() << "Unhandled type " << t.getAsString() << "\n";
      //  t.dump();
      //  TODO Handle the rest as int
      if (mem_access) {
        numMemoryAccesses += 1;
      } else {
        numIntOps += 1;
      }
    }
  }
};
};  // namespace
std::unique_ptr<metacg::MetaData> NumOperationsCollector::computeForDecl(clang::FunctionDecl const* const decl) {
  std::unique_ptr<metacg::NumOperationsMD> result = std::make_unique<metacg::NumOperationsMD>();
  if (decl == nullptr) {
    return {};
  }

  NumOperationsVisitor visitor;
  visitor.TraverseStmt(decl->getBody());
  result->numberOfIntOps = visitor.numIntOps;
  result->numberOfFloatOps = visitor.numFloatOps;
  result->numberOfControlFlowOps = visitor.numControlFlowOps;
  result->numberOfMemoryAccesses = visitor.numMemoryAccesses;
  return result;
}

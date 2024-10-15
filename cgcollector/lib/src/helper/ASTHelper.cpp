#include "helper/ASTHelper.h"
#include "AAUSR.h"
#include <algorithm>
#include <clang/AST/RecursiveASTVisitor.h>
#include <stack>
#include <string>

int getNumStmtsInStmt(clang::Stmt* stmt) {
  int numStmts = 0;
  if (stmt == nullptr) {
    return 0;
  }

  if (llvm::isa<clang::CompoundStmt>(stmt)) {
    numStmts += getNumStmtsInCompoundStmt(llvm::dyn_cast<clang::CompoundStmt>(stmt));
  } else if (llvm::isa<clang::IfStmt>(stmt)) {
    numStmts += getNumStmtsInIfStmt(llvm::dyn_cast<clang::IfStmt>(stmt));
  } else if (llvm::isa<clang::ForStmt>(stmt)) {
    numStmts += getNumStmtsInForStmt(llvm::dyn_cast<clang::ForStmt>(stmt));
  } else if (llvm::isa<clang::WhileStmt>(stmt)) {
    numStmts += getNumStmtsInWhileStmt(llvm::dyn_cast<clang::WhileStmt>(stmt));
  } else if (llvm::isa<clang::CXXForRangeStmt>(stmt)) {
    numStmts += getNumStmtsInCXXForRangeStmt(llvm::dyn_cast<clang::CXXForRangeStmt>(stmt));
  } else if (llvm::isa<clang::DoStmt>(stmt)) {
    numStmts += getNumStmtsInDoStmt(llvm::dyn_cast<clang::DoStmt>(stmt));
  } else if (llvm::isa<clang::CXXTryStmt>(stmt)) {
    numStmts += getNumStmtsInTryStmt(llvm::dyn_cast<clang::CXXTryStmt>(stmt));
  } else if (llvm::isa<clang::SwitchStmt>(stmt)) {
    numStmts += getNumStmtsInSwitchCase(llvm::dyn_cast<clang::SwitchStmt>(stmt));
  } else if (llvm::isa<clang::CaseStmt>(stmt)) {
    numStmts += getNumStmtsInCaseStmt(llvm::dyn_cast<clang::CaseStmt>(stmt));
  } else if (llvm::isa<clang::NullStmt>(stmt)) {
    // The empty statement has no influence on the count
  } else if (llvm::isa<clang::LambdaExpr>(stmt)) {
    // Let's see if we hit that case -- I guess not
  } else {
    if (!llvm::isa<clang::DefaultStmt>(stmt)) {
      numStmts += 1;
    }
  }
  return numStmts;
}

int getNumStmtsInSwitchCase(clang::SwitchStmt* scStmt) {
  int numStmts = 1;

  for (auto child : scStmt->children()) {
    numStmts += getNumStmtsInStmt(child);
  }

  return numStmts;
}

int getNumStmtsInCaseStmt(clang::CaseStmt* cStmt) {
  if (cStmt == nullptr) {
    return 0;
  }

  return getNumStmtsInStmt(cStmt->getSubStmt());
}

int getNumStmtsInTryStmt(clang::CXXTryStmt* tryst) {
  int numStmts = 1;
  numStmts += getNumStmtsInCompoundStmt(tryst->getTryBlock());
  for (size_t i = 0; i < tryst->getNumHandlers(); i++) {
    numStmts += getNumStmtsInCatchStmt(tryst->getHandler(i));
  }
  return numStmts;
}
int getNumStmtsInCatchStmt(clang::CXXCatchStmt* catchst) {
  int numStmts = 1;
  numStmts += getNumStmtsInStmt(catchst->getHandlerBlock());
  return numStmts;
}

int getNumStmtsInCompoundStmt(clang::CompoundStmt* cpst) {
  int numStmts = 0;
  for (clang::CompoundStmt::body_iterator bi = cpst->body_begin(); bi != cpst->body_end(); ++bi) {
    numStmts += getNumStmtsInStmt(*bi);
  }
  return numStmts;
}

int getNumStmtsInIfStmt(clang::IfStmt* is) {
  int numStmts = 1;
  if (is->getThen() != nullptr) {
    numStmts += getNumStmtsInStmt(is->getThen());
  }
  if (is->getElse() != nullptr) {
    numStmts += getNumStmtsInStmt(is->getElse());
  }

  return numStmts;
}

int getNumStmtsInForStmt(clang::ForStmt* fs) {
  int numStmts = 1;
  if (fs->getBody() != nullptr) {
    numStmts += getNumStmtsInStmt(fs->getBody());
  }
  return numStmts;
}

int getNumStmtsInWhileStmt(clang::WhileStmt* ws) {
  int numStmts = 1;
  if (ws->getBody() != nullptr) {
    numStmts += getNumStmtsInStmt(ws->getBody());
  }
  return numStmts;
}

int getNumStmtsInCXXForRangeStmt(clang::CXXForRangeStmt* frs) {
  int numStmts = 1;
  if (frs->getBody() != nullptr) {
    numStmts += getNumStmtsInStmt(frs->getBody());
  }
  return numStmts;
}

int getNumStmtsInDoStmt(clang::DoStmt* ds) {
  int numStmts = 1;
  if (ds->getBody() != nullptr) {
    numStmts += getNumStmtsInStmt(ds->getBody());
  }
  return numStmts;
}

class NumConditionalBranchVisitor : public clang::RecursiveASTVisitor<NumConditionalBranchVisitor> {
 public:
  int count = 0;

  bool shouldVisitTemplateInstantiations() const { return true; }

  bool VisitStmt(clang::Stmt* stmt) {
    switch (stmt->getStmtClass()) {
      case clang::Stmt::IfStmtClass:
      case clang::Stmt::ConditionalOperatorClass:        // ? operator
      case clang::Stmt::BinaryConditionalOperatorClass:  // GNU extension
      case clang::Stmt::WhileStmtClass:
      case clang::Stmt::DoStmtClass:
      case clang::Stmt::ForStmtClass:
      case clang::Stmt::CXXForRangeStmtClass:
      case clang::Stmt::SwitchStmtClass:
        count += 1;
        break;
      default:
        break;
    }
    return true;
  }
};

int getNumConditionalBranchesInStmt(clang::Stmt* s) {
  if (s == nullptr) {
    return 0;
  }
  NumConditionalBranchVisitor visitor;
  visitor.TraverseStmt(s);
  // visitor.visitStmt(s);
  return visitor.count;
}

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
      // std::cerr << "Unhandled type " << t.getAsString() << "\n";
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

NumOperations getNumOperationsInStmt(clang::Stmt* s) {
  if (s == nullptr) {
    return {};
  }
  NumOperations result;
  NumOperationsVisitor visitor;
  visitor.TraverseStmt(s);
  result.numberOfIntOps = visitor.numIntOps;
  result.numberOfFloatOps = visitor.numFloatOps;
  result.numberOfControlFlowOps = visitor.numControlFlowOps;
  result.numberOfMemoryAccesses = visitor.numMemoryAccesses;
  return result;
}

class LoopDepthVisitor : public clang::RecursiveASTVisitor<LoopDepthVisitor> {
 private:
  int cur_loop_depth = 0;

 public:
  int maxdepth = 0;

  bool shouldVisitTemplateInstantiations() const { return true; }

  bool TraverseDoStmt(clang::DoStmt* s, [[maybe_unused]] DataRecursionQueue* q = nullptr) {
    cur_loop_depth++;
    maxdepth = std::max(maxdepth, cur_loop_depth);
    const bool result = RecursiveASTVisitor::TraverseDoStmt(s, nullptr);
    cur_loop_depth--;
    return result;
  }

  bool TraverseWhileStmt(clang::WhileStmt* s, [[maybe_unused]] DataRecursionQueue* q = nullptr) {
    cur_loop_depth++;
    maxdepth = std::max(maxdepth, cur_loop_depth);
    const bool result = RecursiveASTVisitor::TraverseWhileStmt(s, nullptr);
    cur_loop_depth--;
    return result;
  }

  bool TraverseForStmt(clang::ForStmt* s, [[maybe_unused]] DataRecursionQueue* q = nullptr) {
    cur_loop_depth++;
    maxdepth = std::max(maxdepth, cur_loop_depth);
    const bool result = RecursiveASTVisitor::TraverseForStmt(s, nullptr);
    cur_loop_depth--;
    return result;
  }

  bool TraverseCXXForRangeStmt(clang::CXXForRangeStmt* s, [[maybe_unused]] DataRecursionQueue* q = nullptr) {
    cur_loop_depth++;
    maxdepth = std::max(maxdepth, cur_loop_depth);
    const bool result = RecursiveASTVisitor::TraverseCXXForRangeStmt(s, nullptr);
    cur_loop_depth--;
    return result;
  }
};

int getLoopDepthInStmt(clang::Stmt* s) {
  if (s == nullptr) {
    return 0;
  }
  LoopDepthVisitor visitor;
  visitor.TraverseStmt(s);
  return visitor.maxdepth;
}

class CallDepthVisitor : public clang::RecursiveASTVisitor<CallDepthVisitor> {
 private:
  int cur_loop_depth = 0;

 public:
  llvm::SmallDenseMap<const clang::CallExpr*, int, 16> calls;

  bool shouldVisitTemplateInstantiations() const { return true; }

  bool TraverseDoStmt(clang::DoStmt* s, [[maybe_unused]] DataRecursionQueue* q = nullptr) {
    cur_loop_depth++;
    const bool result = RecursiveASTVisitor::TraverseDoStmt(s, nullptr);
    cur_loop_depth--;
    return result;
  }

  bool TraverseWhileStmt(clang::WhileStmt* s, [[maybe_unused]] DataRecursionQueue* q = nullptr) {
    cur_loop_depth++;
    const bool result = RecursiveASTVisitor::TraverseWhileStmt(s, nullptr);
    cur_loop_depth--;
    return result;
  }

  bool TraverseForStmt(clang::ForStmt* s, [[maybe_unused]] DataRecursionQueue* q = nullptr) {
    cur_loop_depth++;
    const bool result = RecursiveASTVisitor::TraverseForStmt(s, nullptr);
    cur_loop_depth--;
    return result;
  }

  bool TraverseCXXForRangeStmt(clang::CXXForRangeStmt* s, [[maybe_unused]] DataRecursionQueue* q = nullptr) {
    cur_loop_depth++;
    const bool result = RecursiveASTVisitor::TraverseCXXForRangeStmt(s, nullptr);
    cur_loop_depth--;
    return result;
  }

  bool VisitCallExpr(clang::CallExpr* ce) {
    assert(calls.count(ce) == 0);
    calls.try_emplace(ce, cur_loop_depth);
    return true;
  }
};

llvm::SmallDenseMap<const clang::CallExpr*, int, 16> getCallDepthsInStmt(clang::Stmt* s) {
  if (s == nullptr) {
    return llvm::SmallDenseMap<const clang::CallExpr*, int, 16>();
  }
  CallDepthVisitor visitor;
  visitor.TraverseStmt(s);
  return visitor.calls;
}

class EstimatedCallCountVisitor : public clang::RecursiveASTVisitor<EstimatedCallCountVisitor> {
 private:
  const clang::SourceManager& SM;
  std::vector<std::string> parents;
  std::vector<double> parentsCallMult;
  bool TraverseStmtCheckNull(clang::Stmt* S, [[maybe_unused]] DataRecursionQueue* Queue = nullptr) {
    if (!S) {
      return true;
    }
    return TraverseStmt(S, nullptr);
  }

  // Returns a unique string representation for an Expr or Stmt defining a BB/Scope. Used for identification and json
  // serialisation
  std::string getStrRepr(clang::Expr* E) { return implementation::generateUSRForConstructInFunction(E, SM); }
  std::string getStrRepr(clang::Stmt* S) { return implementation::generateUSRForConstructInFunction(S, SM); }

  class ParentManager {
    EstimatedCallCountVisitor* parent;

   public:
    ParentManager(EstimatedCallCountVisitor* parent, std::string ID) : parent(parent) {
      parent->parents.emplace_back(std::move(ID));
    }

    ~ParentManager() { parent->parents.pop_back(); }
    ParentManager(ParentManager& other) = default;                   // Copy Constructor
    ParentManager& operator=(const ParentManager& other) = default;  // Copy Assignment
    ParentManager(ParentManager&& other) = default;                  // Move Constructor
    ParentManager& operator=(ParentManager&& other) = default;       // Move Assign
  };

 public:
  CallCountEstimation calls;
  // Cost Model Parameters, Configured via command line parameters
  const double loopCount;
  const double ifTrueChance;
  const double ifFalseChance;
  const double catchChance;
  double switchCaseCountFactor;

  bool shouldVisitTemplateInstantiations() const { return true; }

  bool TraverseIfStmt(clang::IfStmt* S, [[maybe_unused]] DataRecursionQueue* Queue = nullptr) {
    parentsCallMult.push_back(1.0);
    bool result = TraverseStmtCheckNull(S->getCond());
    result = result && TraverseStmtCheckNull(S->getInit());
    result = result && TraverseStmtCheckNull(S->getConditionVariableDeclStmt());

    if (S->getThen()) {
      const ParentManager manager(this, getStrRepr(S->getThen()));
      parentsCallMult.back() = ifTrueChance;
      result = result && TraverseStmtCheckNull(S->getThen());
    }
    if (S->getElse()) {
      const ParentManager manager(this, getStrRepr(S->getElse()));
      parentsCallMult.back() = ifFalseChance;
      result = result && TraverseStmtCheckNull(S->getElse());
    }

    parentsCallMult.pop_back();
    return result;
  }

  bool TraverseConditionalOperator(clang::ConditionalOperator* S,
                                   [[maybe_unused]] DataRecursionQueue* Queue = nullptr) {
    parentsCallMult.push_back(1.0);
    bool result = TraverseStmtCheckNull(S->getCond());

    if (S->getTrueExpr()) {
      const ParentManager manager(this, getStrRepr(S->getTrueExpr()));
      parentsCallMult.back() = ifTrueChance;
      result = result && TraverseStmtCheckNull(S->getTrueExpr());
    }
    if (S->getFalseExpr()) {
      const ParentManager manager(this, getStrRepr(S->getFalseExpr()));
      parentsCallMult.back() = ifFalseChance;
      result = result && TraverseStmtCheckNull(S->getFalseExpr());
    }

    parentsCallMult.pop_back();
    return result;
  }

  bool TraverseBinaryConditionalOperator(clang::BinaryConditionalOperator* S,
                                         [[maybe_unused]] DataRecursionQueue* Queue = nullptr) {
    assert(S->getCond());

    parentsCallMult.push_back(1.0);
    bool result = TraverseStmtCheckNull(S->getCond());

    // The true expr does not get evaluated again, so we do not visit it
    if (S->getFalseExpr()) {
      const ParentManager manager(this, getStrRepr(S->getFalseExpr()));
      parentsCallMult.back() = ifFalseChance;
      result = result && TraverseStmtCheckNull(S->getFalseExpr());
    }
    parentsCallMult.pop_back();
    return result;
  }

  bool TraverseWhileStmt(clang::WhileStmt* S, [[maybe_unused]] DataRecursionQueue* Queue = nullptr) {
    parentsCallMult.push_back(1.0);
    bool result = TraverseStmtCheckNull(S->getCond());
    result = result && TraverseStmtCheckNull(S->getConditionVariableDeclStmt());
    {
      const ParentManager manager(this, getStrRepr(S));
      parentsCallMult.back() = loopCount;
      result = result && TraverseStmtCheckNull(S->getCond());
      result = result && TraverseStmtCheckNull(S->getConditionVariableDeclStmt());
      result = result && TraverseStmtCheckNull(S->getBody());
    }
    parentsCallMult.pop_back();
    return result;
  }

  bool TraverseDoStmt(clang::DoStmt* S, [[maybe_unused]] DataRecursionQueue* Queue = nullptr) {
    parentsCallMult.push_back(1.0);
    bool result = TraverseStmtCheckNull(S->getBody());
    {
      const ParentManager manager(this, getStrRepr(S));
      parentsCallMult.back() = loopCount;
      result = result && TraverseStmtCheckNull(S->getBody());
      result = result && TraverseStmtCheckNull(S->getCond());
    }
    parentsCallMult.pop_back();
    return result;
  }

  bool TraverseForStmt(clang::ForStmt* S, [[maybe_unused]] DataRecursionQueue* q = nullptr) {
    parentsCallMult.push_back(1.0);
    bool result = TraverseStmtCheckNull(S->getInit());
    result = result && TraverseStmtCheckNull(S->getCond());
    result = result && TraverseStmtCheckNull(const_cast<clang::DeclStmt*>(S->getConditionVariableDeclStmt()));
    {
      const ParentManager manager(this, getStrRepr(S));
      parentsCallMult.back() = loopCount;
      result = result && TraverseStmtCheckNull(S->getCond());
      result = result && TraverseStmtCheckNull(const_cast<clang::DeclStmt*>(S->getConditionVariableDeclStmt()));
      result = result && TraverseStmtCheckNull(const_cast<clang::DeclStmt*>(S->getConditionVariableDeclStmt()));
      result = result && TraverseStmtCheckNull(S->getBody());
      result = result && TraverseStmtCheckNull(S->getInc());
    }
    parentsCallMult.pop_back();
    return result;
  }

  bool TraverseCXXForRangeStmt(clang::CXXForRangeStmt* S, [[maybe_unused]] DataRecursionQueue* q = nullptr) {
    parentsCallMult.push_back(1.0);
    bool result = TraverseStmtCheckNull(S->getInit());
    result = result && TraverseStmtCheckNull(S->getRangeStmt());
    result = result && TraverseStmtCheckNull(S->getBeginStmt());
    result = result && TraverseStmtCheckNull(S->getEndStmt());
    result = result && TraverseStmtCheckNull(S->getCond());
    {
      const ParentManager manager(this, getStrRepr(S));
      parentsCallMult.back() = loopCount;
      result = result && TraverseStmtCheckNull(S->getBody());
      result = result && TraverseStmtCheckNull(S->getInc());
      result = result && TraverseStmtCheckNull(S->getCond());
    }
    parentsCallMult.pop_back();
    return result;
  }

  bool TraverseSwitchStmt(clang::SwitchStmt* S, [[maybe_unused]] DataRecursionQueue* q = nullptr) {
    int switchCount = 0;
    for (auto i = S->getSwitchCaseList(); i; i = i->getNextSwitchCase()) {
      switchCount++;
    }
    switchCaseCountFactor = 1.0 / switchCount;
    const bool result = RecursiveASTVisitor::TraverseSwitchStmt(S, nullptr);
    switchCaseCountFactor = 1.0;
    return result;
  }

  bool TraverseCaseStmt(clang::CaseStmt* S, [[maybe_unused]] DataRecursionQueue* q = nullptr) {
    parentsCallMult.push_back(switchCaseCountFactor);
    const ParentManager manager(this, getStrRepr(S));
    const bool result = RecursiveASTVisitor::TraverseCaseStmt(S, nullptr);
    parentsCallMult.pop_back();
    return result;
  }

  bool TraverseDefaultStmt(clang::DefaultStmt* S, [[maybe_unused]] DataRecursionQueue* q = nullptr) {
    parentsCallMult.push_back(switchCaseCountFactor);
    const ParentManager manager(this, getStrRepr(S));
    const bool result = RecursiveASTVisitor::TraverseDefaultStmt(S, nullptr);
    parentsCallMult.pop_back();
    return result;
  }

  bool TraverseCXXCatchStmt(clang::CXXCatchStmt* S, [[maybe_unused]] DataRecursionQueue* q = nullptr) {
    parentsCallMult.push_back(catchChance);
    const ParentManager manager(this, getStrRepr(S));
    const bool result = TraverseStmtCheckNull(S->getHandlerBlock(), nullptr);
    parentsCallMult.pop_back();
    return result;
  }

  bool VisitCallExpr(clang::CallExpr* ce) {
    calls[ce].emplace(parents, parentsCallMult);
    return true;
  }
  EstimatedCallCountVisitor(const clang::SourceManager& manager, float loopCount, float trueChance, float falseChance,
                            float exceptionChance)
      : SM(manager),
        loopCount(loopCount),
        ifTrueChance(trueChance),
        ifFalseChance(falseChance),
        catchChance(exceptionChance) {}
};

CallCountEstimation getEstimatedCallCountInStmt(clang::Stmt* s, const clang::SourceManager& SM, float loopCount,
                                                float trueChance, float falseChance, float exceptionChance) {
  if (s == nullptr) {
    return {};
  }
  EstimatedCallCountVisitor visitor(SM, loopCount, trueChance, falseChance, exceptionChance);
  visitor.TraverseStmt(s);
  return visitor.calls;
}

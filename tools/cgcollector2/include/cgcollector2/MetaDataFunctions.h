/**
* File: MetaDataFunctions.h
* License: Part of the MetaCG project. Licensed under BSD 3 clause license. See LICENSE.txt file at
* https://github.com/tudasc/metacg/LICENSE.txt
 */
#ifndef CGCOLLECTOR_HELPER_ASTHELPER_H
#define CGCOLLECTOR_HELPER_ASTHELPER_H

#include "clang/AST/Stmt.h"
#include "clang/AST/StmtCXX.h"

int getNumStmtsInStmt(clang::Stmt* stmt);
int getNumStmtsInCompoundStmt(clang::CompoundStmt* cpst);
int getNumStmtsInIfStmt(clang::IfStmt* is);
int getNumStmtsInForStmt(clang::ForStmt* fs);
int getNumStmtsInWhileStmt(clang::WhileStmt* ws);
int getNumStmtsInCXXForRangeStmt(clang::CXXForRangeStmt* frs);
int getNumStmtsInDoStmt(clang::DoStmt* ds);
int getNumStmtsInTryStmt(clang::CXXTryStmt* tryst);
int getNumStmtsInCatchStmt(clang::CXXCatchStmt* catchst);
int getNumStmtsInSwitchCase(clang::SwitchStmt* scStmt);
int getNumStmtsInCaseStmt(clang::CaseStmt* cStmt);

int getNumConditionalBranchesInStmt(clang::Stmt* s);

struct NumOperations {
  int numberOfIntOps = 0;
  int numberOfFloatOps = 0;
  int numberOfControlFlowOps = 0;
  int numberOfMemoryAccesses = 0;
};

NumOperations getNumOperationsInStmt(clang::Stmt* s);

int getLoopDepthInStmt(clang::Stmt* s);

llvm::SmallDenseMap<const clang::CallExpr*, int, 16> getCallDepthsInStmt(clang::Stmt* s);

#endif

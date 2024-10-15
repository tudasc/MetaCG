#ifndef CGCOLLECTOR_HELPER_ASTHELPER_H
#define CGCOLLECTOR_HELPER_ASTHELPER_H

#include "clang/AST/Stmt.h"
#include "clang/AST/StmtCXX.h"
#include <set>
#include <string>
#include <vector>

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

/**
 * Information collected for the call count estimation.
 * It maps call expressions to their parents and a factor of how their parents influence how often the call expression
 * gets executed.
 * A call expression can have multiple parents to support the implementation of BB/Scope analysis.
 * For example the condition in a loop is assumed to be in the bb containing the loop, and also being in
 * the bb that represents the loop body
 * std::vector<std::string> is the chain of parent bbs ("" is the function entry)
 * std::vector<double> is the chain of their estimated impact on the call count
 */
using CallCountEstimation =
    std::map<const clang::CallExpr*, std::set<std::pair<std::vector<std::string>, std::vector<double>>>>;

CallCountEstimation getEstimatedCallCountInStmt(clang::Stmt* s, const clang::SourceManager& SM, float loopCount,
                                                float trueChance, float falseChance, float exceptionChance);

#endif

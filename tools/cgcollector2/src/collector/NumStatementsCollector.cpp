/**
 * File: NumStatementsCollector.cpp
 * License: Part of the MetaCG project. Licensed under BSD 3 clause license. See LICENSE.txt file at
 * https://github.com/tudasc/metacg/LICENSE.txt
 */

#include "cgcollector2/collector/NumStatementsCollector.h"
#include <clang/AST/RecursiveASTVisitor.h>

namespace{

int getNumStmtsInStmt(clang::Stmt* stmt) ;

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

int getNumStmtsInCompoundStmt(clang::CompoundStmt* cpst) {
  int numStmts = 0;
  for (clang::CompoundStmt::body_iterator bi = cpst->body_begin(); bi != cpst->body_end(); ++bi) {
    numStmts += getNumStmtsInStmt(*bi);
  }
  return numStmts;
}

int getNumStmtsInCatchStmt(clang::CXXCatchStmt* catchst) {
  int numStmts = 1;
  numStmts += getNumStmtsInStmt(catchst->getHandlerBlock());
  return numStmts;
}

int getNumStmtsInTryStmt(clang::CXXTryStmt* tryst) {
  int numStmts = 1;
  numStmts += getNumStmtsInCompoundStmt(tryst->getTryBlock());
  for (size_t i = 0; i < tryst->getNumHandlers(); i++) {
    numStmts += getNumStmtsInCatchStmt(tryst->getHandler(i));
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

}

std::unique_ptr<metacg::MetaData> NumberOfStatementsCollector::computeForDecl(const clang::FunctionDecl* const functionDecl) {
  std::unique_ptr<metacg::NumStatementsMD> result = std::make_unique<metacg::NumStatementsMD>();
  result->setNumberOfStatements(getNumStmtsInStmt(functionDecl->getBody()));
  return result;
}

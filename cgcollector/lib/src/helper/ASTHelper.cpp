/*
 * currently this is a copy of the ASTHelper of clangOfflineCGCollector
 */

#include "helper/ASTHelper.h"

int getNumStmtsInStmt(clang::Stmt *d) {
  int numStmts = 0;
  if (d == nullptr) {
    return 0;
  }

  if (llvm::isa<clang::CompoundStmt>(d)) {
    numStmts += getNumStmtsInCompoundStmt(llvm::dyn_cast<clang::CompoundStmt>(d));
  } else if (llvm::isa<clang::IfStmt>(d)) {
    numStmts += getNumStmtsInIfStmt(llvm::dyn_cast<clang::IfStmt>(d));
  } else if (llvm::isa<clang::ForStmt>(d)) {
    numStmts += getNumStmtsInForStmt(llvm::dyn_cast<clang::ForStmt>(d));
  } else if (llvm::isa<clang::WhileStmt>(d)) {
    numStmts += getNumStmtsInWhileStmt(llvm::dyn_cast<clang::WhileStmt>(d));
  } else if (llvm::isa<clang::CXXForRangeStmt>(d)) {
    numStmts += getNumStmtsInCXXForRangeStmt(llvm::dyn_cast<clang::CXXForRangeStmt>(d));
  } else if (llvm::isa<clang::DoStmt>(d)) {
    numStmts += getNumStmtsInDoStmt(llvm::dyn_cast<clang::DoStmt>(d));
  } else if (llvm::isa<clang::CXXTryStmt>(d)) {
    numStmts += getNumStmtsInTryStmt(llvm::dyn_cast<clang::CXXTryStmt>(d));
  } else if (llvm::isa<clang::SwitchStmt>(d)) {
    numStmts += getNumStmtsInSwitchCase(llvm::dyn_cast<clang::SwitchStmt>(d));
  } else if (llvm::isa<clang::CaseStmt>(d)) {
    numStmts += getNumStmtsInCaseStmt(llvm::dyn_cast<clang::CaseStmt>(d));
  } else {
    if (! llvm::isa<clang::DefaultStmt>(d)) {
      numStmts += 1;
    }
  }
  return numStmts;
}

int getNumStmtsInSwitchCase(clang::SwitchStmt *scStmt) {
  int numStmts = 1;

  for (auto child : scStmt->children()) {
    numStmts += getNumStmtsInStmt(child);
  }

  return numStmts;
}

int getNumStmtsInCaseStmt(clang::CaseStmt *cStmt){
  if (cStmt == nullptr) {
    return 0;
  }

  return getNumStmtsInStmt(cStmt->getSubStmt());
}

int getNumStmtsInTryStmt(clang::CXXTryStmt *tryst) {
  int numStmts = 1;
  numStmts += getNumStmtsInCompoundStmt(tryst->getTryBlock());
  for (size_t i = 0; i < tryst->getNumHandlers(); i++) {
    numStmts += getNumStmtsInCatchStmt(tryst->getHandler(i));
  }
  return numStmts;
}
int getNumStmtsInCatchStmt(clang::CXXCatchStmt *catchst) {
  int numStmts = 1;
  numStmts += getNumStmtsInStmt(catchst->getHandlerBlock());
  return numStmts;
}

int getNumStmtsInCompoundStmt(clang::CompoundStmt *cpst) {
  int numStmts = 0;
  for (clang::CompoundStmt::body_iterator bi = cpst->body_begin(); bi != cpst->body_end(); ++bi) {
    numStmts += getNumStmtsInStmt(*bi);
  }
  return numStmts;
}

int getNumStmtsInIfStmt(clang::IfStmt *is) {
  int numStmts = 1;
  if (is->getThen() != nullptr) {
    numStmts += getNumStmtsInStmt(is->getThen());
  }
  if (is->getElse() != nullptr) {
    numStmts += getNumStmtsInStmt(is->getElse());
  }

  return numStmts;
}

int getNumStmtsInForStmt(clang::ForStmt *fs) {
  int numStmts = 1;
  if (fs->getBody() != nullptr) {
    numStmts += getNumStmtsInStmt(fs->getBody());
  }
  return numStmts;
}

int getNumStmtsInWhileStmt(clang::WhileStmt *ws) {
  int numStmts = 1;
  if (ws->getBody() != nullptr) {
    numStmts += getNumStmtsInStmt(ws->getBody());
  }
  return numStmts;
}

int getNumStmtsInCXXForRangeStmt(clang::CXXForRangeStmt *frs) {
  int numStmts = 1;
  if (frs->getBody() != nullptr) {
    numStmts += getNumStmtsInStmt(frs->getBody());
  }
  return numStmts;
}

int getNumStmtsInDoStmt(clang::DoStmt *ds) {
  int numStmts = 1;
  if (ds->getBody() != nullptr) {
    numStmts += getNumStmtsInStmt(ds->getBody());
  }
  return numStmts;
}

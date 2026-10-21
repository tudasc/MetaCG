//
// Created by tim on 21.10.26.
//

#include "cgcollector2/collector/NumConditionalBranchCollector.h"
#include <clang/AST/RecursiveASTVisitor.h>
namespace {
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

};
std::unique_ptr<metacg::MetaData> NumConditionalBranchCollector::computeForDecl(clang::FunctionDecl const* const decl) {
  std::unique_ptr<metacg::NumConditionalBranchMD> result = std::make_unique<metacg::NumConditionalBranchMD>();
  result->numConditionalBranches = getNumConditionalBranchesInStmt(decl->getBody());
  return result;
}


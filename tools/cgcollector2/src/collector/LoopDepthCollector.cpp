//
// Created by tim on 21.10.26.
//

#include "cgcollector2/collector/LoopDepthCollector.h"
#include <clang/AST/RecursiveASTVisitor.h>

namespace {
class LoopDepthVisitor : public clang::RecursiveASTVisitor<LoopDepthVisitor> {
private:
  int cur_loop_depth = 0;

public:
  int maxdepth = 0;

  bool shouldVisitTemplateInstantiations() const { return true; }

  bool TraverseDoStmt(clang::DoStmt* s, DataRecursionQueue* q = nullptr) {
    (void)q;
    cur_loop_depth++;
    maxdepth = std::max(maxdepth, cur_loop_depth);
    bool result = RecursiveASTVisitor::TraverseDoStmt(s, nullptr);
    cur_loop_depth--;
    return result;
  }

  bool TraverseWhileStmt(clang::WhileStmt* s, DataRecursionQueue* q = nullptr) {
    (void)q;
    cur_loop_depth++;
    maxdepth = std::max(maxdepth, cur_loop_depth);
    bool result = RecursiveASTVisitor::TraverseWhileStmt(s, nullptr);
    cur_loop_depth--;
    return result;
  }

  bool TraverseForStmt(clang::ForStmt* s, DataRecursionQueue* q = nullptr) {
    (void)q;
    cur_loop_depth++;
    maxdepth = std::max(maxdepth, cur_loop_depth);
    bool result = RecursiveASTVisitor::TraverseForStmt(s, nullptr);
    cur_loop_depth--;
    return result;
  }

  bool TraverseCXXForRangeStmt(clang::CXXForRangeStmt* s, DataRecursionQueue* q = nullptr) {
    (void)q;
    cur_loop_depth++;
    maxdepth = std::max(maxdepth, cur_loop_depth);
    bool result = RecursiveASTVisitor::TraverseCXXForRangeStmt(s, nullptr);
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
}

std::unique_ptr<metacg::MetaData> LoopDepthCollector::computeForDecl(clang::FunctionDecl const* const decl) {
  std::unique_ptr<metacg::LoopDepthMD> result = std::make_unique<metacg::LoopDepthMD>();
  result->loopDepth = getLoopDepthInStmt(decl->getBody());
  return result;
}

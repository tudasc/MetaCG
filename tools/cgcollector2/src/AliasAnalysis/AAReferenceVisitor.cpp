#include "aliasAnalysis/AAReferenceVisitor.h"
#include "aliasAnalysis/AAReferenceVisitorBase.h"
#include "aliasAnalysis/AAUSR.h"

class ReferenceVisitor : public ReferenceVisitorBase<ReferenceVisitor> {
 public:
  ReferenceVisitor(const clang::ASTContext* ctx, clang::FunctionDecl* ParentFunctionDecl);

  bool VisitMemberExpr(clang::MemberExpr* ME);

  bool VisitCallExpr(clang::CallExpr* CE);

  bool VisitCXXConstructExpr(clang::CXXConstructExpr* CE);

  bool VisitDeclRefExpr(clang::DeclRefExpr* DRE);

  bool VisitCXXThisExpr(clang::CXXThisExpr* TE);

  // TODO:Needs more investigation about how typeid is implemented in clang
  // bool VisitCXXTypeidExpr(clang::CXXTypeidExpr *TE);

  bool VisitCXXNewExpr(clang::CXXNewExpr* NE);

  bool VisitMaterializeTemporaryExpr(clang::MaterializeTemporaryExpr* MTE);

  bool TraverseCXXMemberCallExpr(clang::CXXMemberCallExpr* CE, [[maybe_unused]] DataRecursionQueue* = nullptr);

  bool TraverseCXXPseudoDestructorExpr(clang::CXXPseudoDestructorExpr*, [[maybe_unused]] DataRecursionQueue* = nullptr);

  VectorType<ObjectNameDereference> Refs;

 private:
  clang::FunctionDecl* ParentFunctionDecl;
};

class ReturnReferenceVisitor : public ReferenceVisitorBase<ReturnReferenceVisitor> {
 public:
  ReturnReferenceVisitor(const clang::ASTContext* ctx, clang::FunctionDecl* ParentFunctionDecl);

  bool VisitReturnStmt(clang::ReturnStmt* RS);

  VectorType<StringType> RefsInReturnStmts;

 private:
  clang::FunctionDecl* ParentFunctionDecl;
};

bool ReturnReferenceVisitor::VisitReturnStmt(clang::ReturnStmt* RS) {
  const auto Referenced = getReferencedDecls(RS->getRetValue(), CTX, ParentFunctionDecl);
  for (const auto& R : Referenced) {
    RefsInReturnStmts.push_back(R.GetStringRepr());
  }
  return true;
}

ReturnReferenceVisitor::ReturnReferenceVisitor(const clang::ASTContext* ctx, clang::FunctionDecl* ParentFunctionDecl)
    : ReferenceVisitorBase(ctx), ParentFunctionDecl(ParentFunctionDecl) {}

class DeclsVisitor : public ReferenceVisitorBase<DeclsVisitor> {
 public:
  explicit DeclsVisitor(const clang::ASTContext* ctx);

  bool VisitVarDecl(clang::VarDecl* VD);

  bool TraverseTypeLoc(clang::TypeLoc);

  VectorType<StringType> Decls;
};

bool DeclsVisitor::VisitVarDecl(clang::VarDecl* VD) {
  ObjectName Obj(VD);
  Decls.emplace_back(Obj.getStringRepr());
  return true;
}

DeclsVisitor::DeclsVisitor(const clang::ASTContext* ctx) : ReferenceVisitorBase(ctx) {}

bool DeclsVisitor::TraverseTypeLoc(clang::TypeLoc) { return true; }

bool ReferenceVisitor::VisitMemberExpr(clang::MemberExpr* ME) {
  const auto Referenced = getReferencedDecls(ME->getBase(), CTX, ParentFunctionDecl);
  for (const auto& Ref : Referenced) {
    auto MR = std::make_shared<ObjectNameMember>(Ref, generateUSRForDecl(ME->getMemberDecl()->getCanonicalDecl()));
    if (ME->isArrow()) {
      MR->DB.DereferenceLevel += 1;
    }
    Refs.emplace_back(MR, DereferenceLevel);
  }
  return true;
}

bool ReferenceVisitor::VisitCallExpr(clang::CallExpr* CE) {
  auto Obj = std::make_shared<ObjectName>(CE, CTX, ParentFunctionDecl);
  Refs.emplace_back(Obj, DereferenceLevel);
  return true;
}

bool ReferenceVisitor::VisitDeclRefExpr(clang::DeclRefExpr* DRE) {
  const auto Obj = std::make_shared<ObjectName>(DRE->getDecl());
  Refs.emplace_back(Obj, DereferenceLevel);
  return true;
}

ReferenceVisitor::ReferenceVisitor(const clang::ASTContext* ctx, clang::FunctionDecl* ParentFunctionDecl)
    : ReferenceVisitorBase(ctx), ParentFunctionDecl(ParentFunctionDecl) {}

bool ReferenceVisitor::VisitCXXThisExpr(clang::CXXThisExpr* TE) {
  const auto Obj = std::make_shared<ObjectName>(TE, ParentFunctionDecl);
  Refs.emplace_back(Obj, DereferenceLevel);
  return true;
}

bool ReferenceVisitor::VisitCXXNewExpr(clang::CXXNewExpr* NE) {
  const auto Obj = std::make_shared<ObjectName>(NE, ParentFunctionDecl);
  // Here we have some special handling. The new expression itself is the allocated object, but if we take a reference
  // to a new expression we are getting the pointer to it. Return the pointer to the new object and not the object
  // itself
  Refs.emplace_back(Obj, DereferenceLevel - 1);
  return true;
}

bool ReferenceVisitor::TraverseCXXMemberCallExpr(clang::CXXMemberCallExpr* CE, DataRecursionQueue*) {
  return WalkUpFromCXXMemberCallExpr(CE);
}

bool ReferenceVisitor::VisitMaterializeTemporaryExpr(clang::MaterializeTemporaryExpr* MTE) {
  const auto Obj = std::make_shared<ObjectName>(MTE, ParentFunctionDecl);
  Refs.emplace_back(Obj, DereferenceLevel);
  return true;
}

bool ReferenceVisitor::VisitCXXConstructExpr(clang::CXXConstructExpr* CE) {
  // TODO: Functions outside of main
  if (ParentFunctionDecl) {
    auto Obj = std::make_shared<ObjectName>(CE, CTX, ParentFunctionDecl);
    Refs.emplace_back(Obj, DereferenceLevel);
  }
  return true;
}

// Skip. They look like explicit member/destructor calls, but are actually noops
bool ReferenceVisitor::TraverseCXXPseudoDestructorExpr(clang::CXXPseudoDestructorExpr*, DataRecursionQueue*) {
  return true;
}

// TODO: Needs more investigation about how Typeid is implemented in clang
// bool ReferenceVisitor::VisitCXXTypeidExpr(clang::CXXTypeidExpr *TE) {
//   const auto Obj = std::make_shared<ObjectName>(TE);
//   Refs.emplace_back(Obj, DereferenceLevel);
//   return true;
// }

VectorType<ObjectNameDereference> getReferencedDecls(clang::Stmt* Expr, const clang::ASTContext* CTX,
                                                     clang::FunctionDecl* ParentFunctionDecl) {
  ReferenceVisitor Visitor(CTX, ParentFunctionDecl);
  Visitor.TraverseStmt(Expr);

  auto Ret = Visitor.Refs;
  std::sort(Ret.begin(), Ret.end());
  Ret.erase(std::unique(Ret.begin(), Ret.end()), Ret.end());
  Ret.shrink_to_fit();
  return Ret;
}

VectorType<StringType> getDecls(clang::Decl* Decl, const clang::ASTContext* CTX) {
  DeclsVisitor Visitor(CTX);
  Visitor.TraverseDecl(Decl);
  auto Ret = Visitor.Decls;
  std::sort(Ret.begin(), Ret.end());
  Ret.erase(std::unique(Ret.begin(), Ret.end()), Ret.end());
  Ret.shrink_to_fit();
  return Ret;
}

VectorType<StringType> getReferencedInReturnStmts(clang::Stmt* Stmt, const clang::ASTContext* CTX,
                                                  clang::FunctionDecl* ParentFunctionDecl) {
  ReturnReferenceVisitor Visitor(CTX, ParentFunctionDecl);
  Visitor.TraverseStmt(Stmt);
  auto Ret = Visitor.RefsInReturnStmts;
  std::sort(Ret.begin(), Ret.end());
  Ret.erase(std::unique(Ret.begin(), Ret.end()), Ret.end());
  Ret.shrink_to_fit();
  return Ret;
}

VectorType<StringType> getReferencedDeclsStr(clang::Stmt* Stmt, const clang::ASTContext* CTX,
                                             clang::FunctionDecl* ParentFunctionDecl) {
  ReferenceVisitor Visitor(CTX, ParentFunctionDecl);
  Visitor.TraverseStmt(Stmt);
  auto& Tmp = Visitor.Refs;
  std::sort(Tmp.begin(), Tmp.end());
  Tmp.erase(std::unique(Tmp.begin(), Tmp.end()), Tmp.end());
  Tmp.shrink_to_fit();
  VectorType<StringType> Ret;
  for (const auto& T : Tmp) {
    Ret.emplace_back(T.GetStringRepr());
  }
  return Ret;
}

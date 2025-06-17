/**
 * File: AAReferenceVisitorBase.h
 * License: Part of the MetaCG project. Licensed under BSD 3 clause license. See LICENSE.txt file at
 * https://github.com/tudasc/metacg/LICENSE.txt
 */

#ifndef CGCOLLECTOR_AAREFERENCEVISITORBASE_H
#define CGCOLLECTOR_AAREFERENCEVISITORBASE_H

#include <clang/AST/Expr.h>
#include <clang/AST/RecursiveASTVisitor.h>

/**
 * Shared base class for ReferenceVisitor, ReturnReferenceVisitor and DeclVisitor.
 * This class has shared code responsible to skip parts of an expression we are not interested in, for example the index
 * of an subscript expression or types that are referenced in it
 * It also manages the counting of Dereference levels
 * @tparam Derived The type of the child class
 */
template <typename Derived>
class ReferenceVisitorBase : public clang::RecursiveASTVisitor<Derived> {
 public:
  using DataRecursionQueue = typename clang::RecursiveASTVisitor<Derived>::DataRecursionQueue;
  const clang::ASTContext* CTX;
  int DereferenceLevel = 0;

  explicit ReferenceVisitorBase(const clang::ASTContext* ctx) : CTX(ctx) { assert(CTX); }

  // This flag should not matter, as we do not run the derived classes on the complete AST, instead directly over the
  // expressions we are interested in
  bool shouldVisitTemplateInstantiations() const { return true; }
  bool shouldWalkTypesOfTypeLocs() const { return false; }
  bool shouldVisitImplicitCode() const { return true; }

#if LLVM_VERSION_MAJOR < 11
  bool TraverseUnaryDeref(clang::UnaryOperator* UO, [[maybe_unused]] DataRecursionQueue* Queue = nullptr) {
    if (!this->WalkUpFromUnaryDeref(UO)) {
      return false;
    }
    DereferenceLevel += 1;
    // No queue to force an immediate visit
    bool RetValue = this->TraverseStmt(UO->getSubExpr());
    DereferenceLevel -= 1;
    return RetValue;
  }

  bool TraverseUnaryAddrOf(clang::UnaryOperator* UO, [[maybe_unused]] DataRecursionQueue* Queue = nullptr) {
    if (!this->WalkUpFromUnaryAddrOf(UO)) {
      return false;
    }
    DereferenceLevel -= 1;
    // No queue here to force an immediate visit
    bool RetValue = this->TraverseStmt(UO->getSubExpr());
    DereferenceLevel += 1;
    return RetValue;
  }
#else
  bool TraverseUnaryOperator(clang::UnaryOperator* UO, [[maybe_unused]] DataRecursionQueue* Queue = nullptr) {
    const auto Opcode = UO->getOpcode();
    if (Opcode == clang::UnaryOperatorKind::UO_AddrOf || Opcode == clang::UnaryOperatorKind::UO_Deref) {
      if (!this->WalkUpFromUnaryOperator(UO)) {
        return false;
      }
      if (Opcode == clang::UnaryOperatorKind::UO_AddrOf) {
        DereferenceLevel -= 1;
      } else /* UO_Deref */ {
        DereferenceLevel += 1;
      }
      // No queue here to force an immediate visit
      bool RetValue = this->TraverseStmt(UO->getSubExpr());
      if (Opcode == clang::UnaryOperatorKind::UO_AddrOf) {
        DereferenceLevel += 1;
      } else /* UO_Deref */ {
        DereferenceLevel -= 1;
      }
      return RetValue;
    }
    return clang::RecursiveASTVisitor<Derived>::TraverseUnaryOperator(UO, Queue);
  }
#endif

  bool TraverseArraySubscriptExpr(clang::ArraySubscriptExpr* Expr, DataRecursionQueue* Queue = nullptr) {
    if (!this->WalkUpFromArraySubscriptExpr(Expr)) {
      return false;
    }
    return this->TraverseStmt(Expr->getBase(), Queue);
  }

  bool TraverseMemberExpr(clang::MemberExpr* Expr, [[maybe_unused]] DataRecursionQueue* Queue = nullptr) {
    return this->WalkUpFromMemberExpr(Expr);
  }

  bool TraverseBinaryOperator(clang::BinaryOperator* BO, [[maybe_unused]] DataRecursionQueue* Queue = nullptr) {
    if (!this->WalkUpFromBinaryOperator(BO)) {
      return false;
    }
    return this->TraverseStmt(BO->getLHS(), Queue);
  }

  bool TraverseCallExpr(clang::CallExpr* CE, [[maybe_unused]] DataRecursionQueue* Queue = nullptr) {
    return this->WalkUpFromCallExpr(CE);
  }

  bool TraverseCXXOperatorCallExpr(clang::CXXOperatorCallExpr* CE,
                                   [[maybe_unused]] DataRecursionQueue* Queue = nullptr) {
    return this->WalkUpFromCXXOperatorCallExpr(CE);
  }

  bool TraverseCXXMemberCallExpr(clang::CXXMemberCallExpr* CE, [[maybe_unused]] DataRecursionQueue* Queue = nullptr) {
    return this->WalkUpFromCXXMemberCallExpr(CE);
  }

  bool TraverseCXXConstructExpr(clang::CXXConstructExpr* CE, [[maybe_unused]] DataRecursionQueue* Queue = nullptr) {
    return this->WalkUpFromCXXConstructExpr(CE);
  }

  bool TraverseCXXNewExpr(clang::CXXNewExpr* NE, [[maybe_unused]] DataRecursionQueue* Queue = nullptr) {
    return this->WalkUpFromCXXNewExpr(NE);
  }

  bool TraverseCXXDeleteExpr(clang::CXXDeleteExpr* DE, [[maybe_unused]] DataRecursionQueue* Queue = nullptr) {
    return this->WalkUpFromCXXDeleteExpr(DE);
  }

  bool TraverseConditionalOperator(clang::ConditionalOperator* CO, DataRecursionQueue* Queue = nullptr) {
    if (!this->WalkUpFromConditionalOperator(CO)) {
      return false;
    }
    if (!this->TraverseStmt(CO->getTrueExpr(), Queue)) {
      return false;
    }
    if (!this->TraverseStmt(CO->getFalseExpr(), Queue)) {
      return false;
    }
    return true;
  }

  // We are not interested in types
  bool TraverseTypedefDecl(clang::TypedefDecl*) { return true; }
  bool TraverseTypeAliasDecl(clang::TypeAliasDecl*) { return true; }
  bool TraverseNestedNameSpecifierLoc(clang::NestedNameSpecifierLoc) { return true; }
  bool TraverseTemplateArgumentLoc(const clang::TemplateArgumentLoc&) { return true; }
  bool TraverseDeclarationNameInfo(clang::DeclarationNameInfo) { return true; }
  bool TraverseCXXNoexceptExpr(clang::CXXNoexceptExpr*, DataRecursionQueue* = nullptr) { return true; }

  // We are not interested in nested functions or similar
  bool TraverseCXXRecordDecl(clang::CXXRecordDecl*) { return true; }
  bool TraverseRecordDecl(clang::RecordDecl*) { return true; }

  bool TraverseMaterializeTemporaryExpr(clang::MaterializeTemporaryExpr* MTE,
                                        [[maybe_unused]] DataRecursionQueue* Queue = nullptr) {
    return this->WalkUpFromMaterializeTemporaryExpr(MTE);
  }
};

#endif  // CGCOLLECTOR_AAREFERENCEVISITORBASE_H

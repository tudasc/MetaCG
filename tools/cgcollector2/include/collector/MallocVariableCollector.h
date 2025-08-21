/**
 * File: MallocVariableCollector.cpp
 * License: Part of the MetaCG project. Licensed under BSD 3 clause license. See LICENSE.txt file at
 * https://github.com/tudasc/metacg/LICENSE.txt
 */

#ifndef CGCOLLECTOR2_MALLOCVARIABLECOLLECTOR_H
#define CGCOLLECTOR2_MALLOCVARIABLECOLLECTOR_H

#include "Plugin.h"
#include "metadata/MallocVariableMD.h"
#include <clang/AST/StmtVisitor.h>


struct MallocVariableCollector : public Plugin {
  std::string getPluginName() const final{ return "MallocVariableCollector"; }

  virtual std::unique_ptr<metacg::MetaData> computeForDecl(clang::FunctionDecl const* const decl) {
    std::unique_ptr<metacg::MallocVariableMD> result = std::make_unique<metacg::MallocVariableMD>();

    class MallocFinder : public clang::StmtVisitor<MallocFinder> {
      clang::ASTContext& ctx;
      std::map<std::string, std::string>& allocs;

     public:
      MallocFinder(clang::ASTContext& ctx, std::map<std::string, std::string>& allocs) : ctx(ctx), allocs(allocs){};
      ~MallocFinder() = default;

      void VisitStmt(clang::Stmt* stmt) {
        for (auto s : stmt->children()) {
          if (s) {
            this->Visit(s);
          }
        }
      }

      bool handleFuncCallForVar(const clang::CallExpr* ce, const clang::VarDecl* vd) {
        if (ce->getCalleeDecl()) {
          if (const auto funSym = llvm::dyn_cast<clang::FunctionDecl>(ce->getCalleeDecl())) {
            if (!vd->isLocalVarDecl()) {
              std::cout << "Found " << funSym->getNameAsString() << " call in assignment to " << vd->getNameAsString()
                        << "\n";
              return true;
            }
          }
        }
        return false;
      }

      /**
       * Handles (probably) non-existing cases of declarations for globals
       */
      void VisitDeclStmt(clang::DeclStmt* ds) {
        // *Probably* non existing case in general...
        if (ds->isSingleDecl()) {
          if (const auto d = llvm::dyn_cast<clang::VarDecl>(ds->getSingleDecl())) {
            if (!d->hasInit()) {
              return;
            }
            if (const auto init = llvm::dyn_cast<clang::ExplicitCastExpr>(d->getInit())) {
              if (const auto ce = llvm::dyn_cast<clang::CallExpr>(init->getSubExpr())) {
                if (handleFuncCallForVar(ce, d)) {
                  ds->dumpPretty(ctx);
                  std::cout << "\n\n";

                  std::string stmtStr;
                  llvm::raw_string_ostream oss(stmtStr);
                  const clang::PrintingPolicy pp(ctx.getLangOpts());
                  const int indent = 0;
                  ds->printPretty(oss, nullptr, pp, indent, "\n", &ctx);
                  oss.flush();
                  allocs.insert({d->getNameAsString(), stmtStr});
                }
              }
            }
          }
        }
      }

      /**
       * Handles assignment statements with call to, e.g., "malloc".
       * int * k = (int *) malloc(sizeof(int));
       * */
      void VisitBinaryOperator(clang::BinaryOperator* bo) {
        if (bo->isAssignmentOp()) {
          auto lhs = bo->getLHS();
          auto rhs = bo->getRHS();

          if (!lhs || !rhs) {
            return;
          }

          if (const auto vRef = llvm::dyn_cast<clang::DeclRefExpr>(lhs)) {
            if (const auto fc = llvm::dyn_cast<clang::ExplicitCastExpr>(rhs)) {
              if (const auto ce = llvm::dyn_cast<clang::CallExpr>(fc->getSubExpr())) {
                if (const auto vd = llvm::dyn_cast<clang::VarDecl>(vRef->getDecl())) {
                  handleFuncCallForVar(ce, vd);
                }
              }
            }
            if (const auto ne = llvm::dyn_cast<clang::CXXNewExpr>(rhs)) {
              std::cout << "Found new expression for " << vRef->getDecl()->getNameAsString() << "\n";
              bo->dumpPretty(ctx);
              std::cout << "\n\n";
              std::string stmtStr;
              llvm::raw_string_ostream oss(stmtStr);
              const clang::PrintingPolicy pp(ctx.getLangOpts());
              const int indent = 0;
              bo->printPretty(oss, nullptr, pp, indent, "\n", &ctx);
              oss.flush();
              allocs.insert({vRef->getDecl()->getNameAsString(), stmtStr});
            }
          }
        }
      }
    };

    MallocFinder mf(decl->getASTContext(), result->allocs);
    if (decl->getBody()) {
      mf.Visit(decl->getBody());
    }
    return result;
  }

 public:
  virtual ~MallocVariableCollector() = default;
};

#endif  // CGCOLLECTOR2_MALLOCVARIABLECOLLECTOR_H

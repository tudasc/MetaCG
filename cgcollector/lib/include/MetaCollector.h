#ifndef CGCOLLECTOR_METACOLLECTOR_H
#define CGCOLLECTOR_METACOLLECTOR_H

#include "helper/ASTHelper.h"
#include "helper/common.h"

#include "CallGraph.h"
#include "GlobalCallDepth.h"
#include "MetaInformation.h"
#include "Utils.h"

#include <clang/AST/Decl.h>
#include <clang/AST/StmtVisitor.h>
#include <clang/Basic/SourceManager.h>

#include <map>
#include <memory>

#include <iostream>

class MetaCollector {
  std::string name;

  std::map<std::string, std::unique_ptr<MetaInformation>> values;

  virtual std::unique_ptr<MetaInformation> calculateForFunctionDecl(
      clang::FunctionDecl const *const decl,
      const llvm::DenseMap<const clang::CallExpr *, const clang::Decl *> &calledDecls) = 0;

 protected:
  MetaCollector(std::string name) : name(name) {}

 public:
  void calculateFor(const CallGraph &cg) {
    // for (auto &node : cg) {
    for (const auto &node : cg.getInOrderDecls()) {
      // if (node.first) {
      // if (auto f = llvm::dyn_cast<clang::FunctionDecl>(node.first)) {
      if (auto f = llvm::dyn_cast<clang::FunctionDecl>(node)) {
        auto names = getMangledName(f);
        for (const auto &n : names) {
          values[n] = calculateForFunctionDecl(f, cg.CalledDecls);
        }
        //}
      }
    }
  }

  virtual void addMetaInformationToCompleteJson([[maybe_unused]] nlohmann::json &j,
                                                [[maybe_unused]] int mcgFormatVersion) {}

  const std::map<std::string, std::unique_ptr<MetaInformation>> &getMetaInformation() { return values; }

  std::string getName() { return name; }
};

class NumberOfStatementsCollector final : public MetaCollector {
  std::unique_ptr<MetaInformation> calculateForFunctionDecl(
      clang::FunctionDecl const *const decl,
      const llvm::DenseMap<const clang::CallExpr *, const clang::Decl *> &) override {
    std::unique_ptr<NumberOfStatementsResult> result = std::make_unique<NumberOfStatementsResult>();

    result->numberOfStatements = getNumStmtsInStmt(decl->getBody());

    return result;
  }

 public:
  NumberOfStatementsCollector() : MetaCollector("numStatements") {}
};

class FilePropertyCollector : public MetaCollector {
  std::unique_ptr<MetaInformation> calculateForFunctionDecl(
      clang::FunctionDecl const *const decl,
      const llvm::DenseMap<const clang::CallExpr *, const clang::Decl *> &) override {
    auto result = std::make_unique<FilePropertyResult>();
    const auto sourceLocation = decl->getLocation();
    auto &astCtx = decl->getASTContext();
    const auto fullSrcLoc = astCtx.getFullLoc(sourceLocation);
    const auto fileEntry = fullSrcLoc.getFileEntry();
    if (!fileEntry) {
      return result;
    }
    const auto fileName = fileEntry->getName();
    std::string fileNameStr = fileName.str();

    result->isFromSystemInclude = astCtx.getSourceManager().isInSystemHeader(sourceLocation);

    result->fileOrigin = fileNameStr;
    return result;
  }

 public:
  FilePropertyCollector() : MetaCollector("fileProperties") {}
};

class CodeStatisticsCollector : public MetaCollector {
  std::unique_ptr<MetaInformation> calculateForFunctionDecl(
      clang::FunctionDecl const *const decl,
      const llvm::DenseMap<const clang::CallExpr *, const clang::Decl *> &) override {
    auto result = std::make_unique<CodeStatisticsMetaInformation>();
    for (auto declIter = decl->decls_begin(); declIter != decl->decls_end(); ++declIter) {
      if (const auto varDecl = llvm::dyn_cast<clang::VarDecl>(*declIter)) {
        result->numVars++;
      }
    }
    return result;
  }

 public:
  CodeStatisticsCollector() : MetaCollector("codeStatistics") {}
};

class MallocVariableCollector : public MetaCollector {
  std::unique_ptr<MetaInformation> calculateForFunctionDecl(
      clang::FunctionDecl const *const decl,
      const llvm::DenseMap<const clang::CallExpr *, const clang::Decl *> &) override {
    auto result = std::make_unique<MallocVariableInformation>();
    class MallocFinder : public clang::StmtVisitor<MallocFinder> {
      clang::ASTContext &ctx;
      std::map<std::string, std::string> &allocs;

     public:
      MallocFinder(clang::ASTContext &ctx, std::map<std::string, std::string> &allocs)
          : ctx(ctx), allocs(allocs){};
      ~MallocFinder() = default;

      void VisitStmt(clang::Stmt *stmt) {
        for (auto s : stmt->children()) {
          if (s) {
            this->Visit(s);
          }
        }
      }

      bool handleFuncCallForVar(const clang::CallExpr *ce, const clang::VarDecl *vd) {
        if (ce->getCalleeDecl()) {
        if (const auto funSym = llvm::dyn_cast<clang::FunctionDecl>(ce->getCalleeDecl())) {
          if (!vd->isLocalVarDecl()) {
            std::cout << "Found " << funSym->getNameAsString() << " call in assignment to " << vd->getNameAsString() << "\n";
            return true;
          }}
        }
        return false;
      }

      /**
       * Handles (probably) non-existing cases of declarations for globals
       */
      void VisitDeclStmt(clang::DeclStmt *ds) {
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
                clang::PrintingPolicy pp(ctx.getLangOpts());
                int indent = 0;
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
      void VisitBinaryOperator(clang::BinaryOperator *bo) {
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
                clang::PrintingPolicy pp(ctx.getLangOpts());
                int indent = 0;
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
  MallocVariableCollector() : MetaCollector("mallocCollector") {}
};

class UniqueTypeCollector : public MetaCollector {
  std::set<const clang::Type *> globalTypes;

  virtual std::unique_ptr<MetaInformation> calculateForFunctionDecl(
      clang::FunctionDecl const *const decl,
      const llvm::DenseMap<const clang::CallExpr *, const clang::Decl *> &) override {
    std::set<const clang::Type *> uniqueTypes;

    std::cout << "Processing function " << decl->getNameAsString() << std::endl;

    class DeclRefExprVisitor : public clang::StmtVisitor<DeclRefExprVisitor> {
      std::set<const clang::Type *> &fTypes;

     public:
      DeclRefExprVisitor(std::set<const clang::Type *> &funcTypes) : fTypes(funcTypes) {}

      void VisitStmt(clang::Stmt *stmt) {
        for (const auto s : stmt->children()) {
          if (s) {
            this->Visit(s);
          }
        }
      }

      void VisitDeclStmt(clang::DeclStmt *ds) {
        if (ds->isSingleDecl()) {
          if (const auto decl = llvm::dyn_cast<clang::ValueDecl>(ds->getSingleDecl())) {
            const auto ty = resolveToUnderlyingType(decl->getType().getTypePtr());
            fTypes.insert(ty);
          }
        } else {
          std::cerr << "Found DeclGroup: Not Collecting Types." << std::endl;
        }
      }

      void VisitDeclRefExpr(clang::DeclRefExpr *dre) {
        if (!dre->getDecl()) {
          return ;
          }
          if (const auto decl = dre->getDecl()) {
            const auto ty = resolveToUnderlyingType(decl->getType().getTypePtr());
            if (! (ty->isFunctionType() || ty->isFunctionPointerType())) {
              fTypes.insert(ty);
            }
          }
        }
    };

    if (!decl->hasBody()) {
      return std::make_unique<UniqueTypeInformation>();
    }

    DeclRefExprVisitor dreV(uniqueTypes);
    dreV.Visit(decl->getBody());

    auto uti = std::make_unique<UniqueTypeInformation>();
    uti->numTypes = uniqueTypes.size();
    globalTypes.insert(std::begin(uniqueTypes), std::end(uniqueTypes));
    if (decl->isMain()) {
      uti->numTypes = globalTypes.size();
    }
    return uti;
  }

 public:
  UniqueTypeCollector() : MetaCollector("uniqueTypeCollector") {}
};

class NumConditionalBranchCollector final : public MetaCollector {
 public:
  NumConditionalBranchCollector() : MetaCollector("numConditionalBranches") {}

 private:
  std::unique_ptr<MetaInformation> calculateForFunctionDecl(
      const clang::FunctionDecl *const decl,
      const llvm::DenseMap<const clang::CallExpr *, const clang::Decl *> &) override {
    auto result = std::make_unique<NumOfConditionalBranchesResult>();
    result->numberOfConditionalBranches = getNumConditionalBranchesInStmt(decl->getBody());
    return result;
  }
};

class NumOperationsCollector final : public MetaCollector {
 public:
  NumOperationsCollector() : MetaCollector("numOperations") {}

 private:
  std::unique_ptr<MetaInformation> calculateForFunctionDecl(
      const clang::FunctionDecl *const decl,
      const llvm::DenseMap<const clang::CallExpr *, const clang::Decl *> &) override {
    assert(decl);
    auto result = std::make_unique<NumOperationsResult>();
    const auto counts = getNumOperationsInStmt(decl->getBody());
    result->numberOfIntOps = counts.numberOfIntOps;
    result->numberOfFloatOps = counts.numberOfFloatOps;
    result->numberOfControlFlowOps = counts.numberOfControlFlowOps;
    result->numberOfMemoryAccesses = counts.numberOfMemoryAccesses;
    return result;
  }
};

class LoopDepthCollector final : public MetaCollector {
 public:
  LoopDepthCollector() : MetaCollector("loopDepth") {}

 private:
  std::unique_ptr<MetaInformation> calculateForFunctionDecl(
      const clang::FunctionDecl *const decl,
      const llvm::DenseMap<const clang::CallExpr *, const clang::Decl *> &) override {
    assert(decl);
    auto result = std::make_unique<LoopDepthResult>();
    result->loopDepth = getLoopDepthInStmt(decl->getBody());
    return result;
  }
};

class GlobalLoopDepthCollector final : public MetaCollector {
 public:
  GlobalLoopDepthCollector() : MetaCollector("loopCallDepth") {}
  void addMetaInformationToCompleteJson(nlohmann::json &j, int mcgFormatVersion) override {
    if (mcgFormatVersion > 1) {
      calculateGlobalCallDepth(j, false);
    }
  }

 private:
  std::unique_ptr<MetaInformation> calculateForFunctionDecl(
      const clang::FunctionDecl *const decl,
      const llvm::DenseMap<const clang::CallExpr *, const clang::Decl *> &calledDecls) override {
    assert(decl);
    auto result = std::make_unique<GlobalLoopDepthResult>();
    std::map<std::string, int> calledFunctions;
    const auto callDepths = getCallDepthsInStmt(decl->getBody());
    for (const auto &c : callDepths) {
      const auto i = calledDecls.find(c.first);
      // There are unfortunately some cases where we can not map the call to what is called
      if (i != calledDecls.end()) {
        // TODO: Maybe handle constructors
        if (auto fdecl = llvm::dyn_cast<clang::FunctionDecl>(i->getSecond())) {
          const auto fnames = getMangledName(fdecl);
          for (const auto &fname : fnames) {
            auto fi = calledFunctions.find(fname);
            if (fi == calledFunctions.end()) {
              calledFunctions.emplace(fname, c.second);
            } else {
              fi->second = std::max(fi->second, c.second);
            }
          }
        }
      }
    }
    result->calledFunctions = calledFunctions;
    return result;
  }
};

#endif /* ifndef CGCOLLECTOR_METACOLLECTOR_H */

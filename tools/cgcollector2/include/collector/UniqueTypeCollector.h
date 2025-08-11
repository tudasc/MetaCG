/**
* File: UniqueTypeCollector.h
* License: Part of the MetaCG project. Licensed under BSD 3 clause license. See LICENSE.txt file at
* https://github.com/tudasc/metacg/LICENSE.txt
*/

#ifndef CGCOLLECTOR2_UNIQUETYPECOLLECTOR_H
#define CGCOLLECTOR2_UNIQUETYPECOLLECTOR_H

#include "Plugin.h"
#include "metadata/UniqueTypeMD.h"

struct UniqueTypeCollector : public Plugin {
 public:
  virtual std::string getPluginName() { return key; }

  static constexpr const char* key = "UniqueTypeCollector";
  std::set<const clang::Type*> globalTypes;
  virtual std::unique_ptr<metacg::MetaData> computeForDecl(clang::FunctionDecl const* const decl) {
    std::set<const clang::Type*> uniqueTypes;

    std::cout << "Processing function " << decl->getNameAsString() << std::endl;

    class DeclRefExprVisitor : public clang::StmtVisitor<DeclRefExprVisitor> {
      std::set<const clang::Type*>& fTypes;
      const clang::Type* resolveToUnderlyingType(const clang::Type* ty) {
        if (!ty->isPointerType()) {
          return ty;
          // std::cout << "resolveToUnderlyingType: isPointerType" << std::endl;
        }
        return resolveToUnderlyingType(ty->getPointeeType().getTypePtr());
      }

     public:
      DeclRefExprVisitor(std::set<const clang::Type*>& funcTypes) : fTypes(funcTypes) {}

      void VisitStmt(clang::Stmt* stmt) {
        for (const auto s : stmt->children()) {
          if (s) {
            this->Visit(s);
          }
        }
      }

      void VisitDeclStmt(clang::DeclStmt* ds) {
        if (ds->isSingleDecl()) {
          if (const auto decl = llvm::dyn_cast<clang::ValueDecl>(ds->getSingleDecl())) {
            const auto ty = resolveToUnderlyingType(decl->getType().getTypePtr());
            fTypes.insert(ty);
          }
        } else {
          std::cerr << "Found DeclGroup: Not Collecting Types." << std::endl;
        }
      }

      void VisitDeclRefExpr(clang::DeclRefExpr* dre) {
        if (!dre->getDecl()) {
          return;
        }
        if (const auto decl = dre->getDecl()) {
          const auto ty = resolveToUnderlyingType(decl->getType().getTypePtr());
          if (!(ty->isFunctionType() || ty->isFunctionPointerType())) {
            fTypes.insert(ty);
          }
        }
      }
    };

    if (!decl->hasBody()) {
      return std::make_unique<metacg::UniqueTypeMD>();
    }

    DeclRefExprVisitor dreV(uniqueTypes);
    dreV.Visit(decl->getBody());

    auto uti = std::make_unique<metacg::UniqueTypeMD>();
    uti->numTypes = uniqueTypes.size();
    globalTypes.insert(std::begin(uniqueTypes), std::end(uniqueTypes));
    if (decl->isMain()) {
      uti->numTypes = globalTypes.size();
    }
    return uti;
  }

  virtual ~UniqueTypeCollector() = default;

 private:
};

#endif  // CGCOLLECTOR2_UNIQUETYPECOLLECTOR_H

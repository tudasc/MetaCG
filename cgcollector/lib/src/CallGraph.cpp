//===- CallGraph.cpp - AST-based Call graph -------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
//  This file defines the AST-based CallGraph.
//
//===----------------------------------------------------------------------===//

#include "CallGraph.h"
#include "Utils.h"

#include <clang/AST/Decl.h>
#include <clang/AST/DeclBase.h>
#include <clang/AST/DeclObjC.h>
#include <clang/AST/Expr.h>
#include <clang/AST/ExprObjC.h>
#include <clang/AST/Stmt.h>
#include <clang/AST/StmtVisitor.h>
#include <clang/Basic/IdentifierTable.h>
#include <clang/Basic/LLVM.h>
#include <llvm/ADT/PostOrderIterator.h>
#include <llvm/ADT/STLExtras.h>
#include <llvm/ADT/Statistic.h>
#include <llvm/Support/Casting.h>
#include <llvm/Support/Compiler.h>
#include <llvm/Support/DOTGraphTraits.h>
#include <llvm/Support/GraphWriter.h>
#include <llvm/Support/raw_ostream.h>

#include <cassert>
#include <memory>
#include <string>
#include <unordered_set>

#include <iostream>
#include <sstream>

using namespace clang;

#define DEBUG_TYPE "CallGraph"

STATISTIC(NumObjCCallEdges, "Number of Objective-C method call edges");
STATISTIC(NumBlockCallEdges, "Number of block call edges");

void printNamedDeclToConsole(const Decl *D) {
  std::function<void()> func;
  assert(D);
#ifndef NDEBUG
  auto fd = llvm::dyn_cast<NamedDecl>(D);
  if (fd) {
    std::cout << fd->getNameAsString() << std::endl;
  }
#endif
}

auto Location(Decl *decl, Stmt *expr) {
  const auto &ctx = decl->getASTContext();
  const auto &man = ctx.getSourceManager();
  return expr->getSourceRange().getBegin().printToString(man);
}

/**
 * Helper to retrieve a set of symbols reference in a subtree.
 */
template <typename Filter>
class DeclRefRetriever : public StmtVisitor<DeclRefRetriever<Filter>> {
 public:
  DeclRefRetriever(Filter filt, const std::unordered_set<const VarDecl *> &relevantSymbols, const std::string name = "")
      : filt(filt), relevantSymbols(relevantSymbols), instanceName(name) {}
  void VisitStmt(Stmt *S) { VisitChildren(S); }

  void VisitDeclRefExpr(DeclRefExpr *dre) {
    if (const auto decl = dre->getDecl()) {
      //	  std::cout << "DeclRefRetriever::VisitDeclRefExpr: decl non null" << std::endl;
      if (const auto valueDecl = dyn_cast<ValueDecl>(decl)) {
        const auto ty = resolveToUnderlyingType(valueDecl->getType().getTypePtr());
        const auto inRelevantMemberExpr = [](DeclRefExpr *dre) {
          const auto decl = dre->getDecl();
          auto &ctx = decl->getASTContext();
          auto pMap = ctx.getParents(*dre);
          auto firstParent = pMap.begin();
          if (const auto memExp = dyn_cast<MemberExpr>((*firstParent).get<Stmt>())) {
            return true;
          }
          return false;
        };
        const auto inRelevantSymbols = [&](const ValueDecl *vd) {
          if (const auto vDecl = dyn_cast<VarDecl>(vd)) {
            bool relevant = relevantSymbols.find(vDecl) != relevantSymbols.end();
            return relevant;
          }
          return false;
        };

        bool relevant = ty->isFunctionPointerType() || ty->isFunctionType() || ty->isMemberPointerType() ||
                        inRelevantMemberExpr(dre) || inRelevantSymbols(valueDecl);
        if (!relevant) {
          return;
        }
      }

      if (filt(dre)) {
        return;
      }
      // case 1: variable declaration
      if (isa<VarDecl>(*decl)) {
        // std::cout << "DeclRefRetriever::VisitDeclRefExpr: decl is VarDecl" << std::endl;
        symbols.insert(decl);
      }
      // case 2: function declaration
      if (const auto fd = dyn_cast<FunctionDecl>(decl)) {  // isa<FunctionDecl>(*decl)) {
        symbols.insert(decl);
      }
    }
  }

  const std::unordered_set<Decl *> &getSymbols() { return symbols; }

  void VisitChildren(Stmt *S) {
    for (Stmt *SubStmt : S->children())
      if (SubStmt) {
        this->Visit(SubStmt);
      }
  }

  void printSymbols() const {
    std::cout << "=== DeclRefRetriever " << instanceName << " Symbols ===" << std::endl;
    for (const auto sym : symbols) {
      if (const auto nd = dyn_cast<NamedDecl>(sym)) {
        std::cout << nd->getNameAsString() << std::endl;
      }
    }
  }

 private:
  std::unordered_set<Decl *> symbols;
  Filter filt;
  const std::unordered_set<const VarDecl *> &relevantSymbols;
  std::string instanceName;
};

class FunctionPointerTracer : public StmtVisitor<FunctionPointerTracer> {
 public:
  FunctionPointerTracer(CallGraph *g, const llvm::DenseMap<const Decl *, llvm::SmallSet<const Decl *, 8>> &aliases,
                        CallExpr *ce, llvm::SmallSet<const CallExpr *, 16> closed_ces = {})
      : closedCes(closed_ces), G(g), CE(ce), aliases(aliases) {
    closedCes.insert(ce);
  }
  void VisitChildren(Stmt *S) {
    for (Stmt *subStmt : S->children()) {
      if (subStmt) {
        this->Visit(subStmt);
      }
    }
  }
  void VisitCallExpr(CallExpr *ce) {
    // Visit definition
    RecursiveLookUpCallExpr(ce);
    VisitChildren(ce);
  }
  void RecursiveLookUpCallExpr(CallExpr *ce) {
    if (closedCes.find(ce) != closedCes.end()) {
      return;
    }
//    closedCes.insert(ce);
    if (auto directCallee = ce->getDirectCallee()) {
      const auto fName = directCallee->getNameAsString();
      if (auto body = directCallee->getBody()) {
        {
          std::size_t i = 0;
          for (auto argIter = ce->arg_begin(); argIter != ce->arg_end(); argIter++) {
            if (i < ce->getDirectCallee()->getNumParams()) {
              auto paramDecl = directCallee->getParamDecl(i);
              if (auto refDecl = (*argIter)->getReferencedDeclOfCallee()) {
                aliases[paramDecl].insert(refDecl);
              }
            }
            i++;
          }
        }
        FunctionPointerTracer fpt(G, aliases, ce, closedCes);
        fpt.Visit(body);

        std::size_t i = 0;
        for (auto iter = ce->arg_begin(); iter != ce->arg_end(); iter++) {
          auto directCallee = ce->getDirectCallee();

          VarDecl *varDecl = nullptr;
          // if an adress of operator is used in argument list
          if (UnaryOperator *uo = dyn_cast<UnaryOperator>(*iter)) {
            if (uo->getOpcode() == UO_AddrOf) {
              if (DeclRefExpr *dre = dyn_cast<DeclRefExpr>(uo->getSubExpr())) {
                varDecl = dyn_cast<VarDecl>(dre->getDecl());
              }
            }
          } else if (ImplicitCastExpr *ice = dyn_cast<ImplicitCastExpr>(*iter)) {
            if (DeclRefExpr *dre = dyn_cast<DeclRefExpr>(ice->getSubExpr())) {
              varDecl = dyn_cast<VarDecl>(dre->getDecl());
            }
          }

          if (i < directCallee->getNumParams()) {
            ParmVarDecl *parm_decl = directCallee->getParamDecl(i);
            auto targets = fpt.target_params(parm_decl);
            aliases[varDecl].insert(targets.begin(), targets.end());
          }

          i++;
        }

        auto getAssignedVar = [&](CallExpr *ce, ASTContext &c) -> const Decl * {
          const Decl *result = nullptr;
          auto parents = c.getParents(*ce);
          auto pIt = parents.begin();
          if (auto pDecl = (*pIt).get<Decl>()) {
            result = dyn_cast<VarDecl>(pDecl);
          } else if (auto bo = (*pIt).get<BinaryOperator>()) {
            if (auto lhs = bo->getLHS()) {
              if (auto uo = dyn_cast<UnaryOperator>(lhs)) {
                if (auto ice = dyn_cast<ImplicitCastExpr>(uo->getSubExpr())) {
                  if (auto dre = dyn_cast<DeclRefExpr>(ice->getSubExpr())) {
                    result = dre->getDecl();
                  }
                }
              } else if (auto ice = dyn_cast<ImplicitCastExpr>(lhs)) {
                if (auto dre = dyn_cast<DeclRefExpr>(ice->getSubExpr())) {
                  result = dre->getDecl();
                }
              } else if (auto dre = dyn_cast<DeclRefExpr>(lhs)) {
                result = dre->getDecl();
              }
            }
          }
          return result;
        };

        const auto dStmt = getAssignedVar(ce, ce->getCalleeDecl()->getASTContext());
        const Decl *fVar = dStmt;
        if (fVar) {
          auto tr = fpt.getTargetsReturn();
          aliases[fVar].insert(tr.begin(), tr.end());
        }
      }
    } else {
      if (auto ice = dyn_cast<ImplicitCastExpr>(*ce->children().begin())) {
        if (auto dre = dyn_cast<DeclRefExpr>(ice->getSubExpr())) {
          if (auto decl = dre->getDecl()) {
            llvm::SmallSet<const FunctionDecl *, 16> result;
            FindTargetParams(result, decl, decl);
            for (const auto *fd : result) {
              addCalledDecl(CE->getCalleeDecl(), fd, CE);
            }
          }
        }
      }
    }
  }
  void VisitStmt(Stmt *S) { VisitChildren(S); }
  void VisitBinaryOperator(BinaryOperator *bo) {
    if (bo->isAssignmentOp()) {
      auto lhs = bo->getLHS();
      auto rhs = bo->getRHS();

      DeclRefRetriever lhsDRR([]( [[maybe_unused]] DeclRefExpr *dre) { return false; }, relevantSymbols);
      lhsDRR.Visit(lhs);
      auto lhsSymbols = lhsDRR.getSymbols();
#if 0
      std::cout << "FunctionPointerTracer\n=== LHS ===" << std::endl;
      for (const auto sym : lhsSymbols) {
        if (const auto nd = dyn_cast<NamedDecl>(sym)) {
          std::cout << "| " << nd->getNameAsString() << std::endl;
        }
      }
#endif

      DeclRefRetriever rhsDRR([]( [[maybe_unused]] DeclRefExpr *dre) { return false; }, relevantSymbols);
      rhsDRR.Visit(rhs);
      auto rhsSymbols = rhsDRR.getSymbols();
#if 0
      std::cout << "FunctionPointerTracer\n=== RHS ===" << std::endl;
      for (const auto sym : rhsSymbols) {
        if (const auto nd = dyn_cast<NamedDecl>(sym)) {
          std::cout << "| " << nd->getNameAsString() << std::endl;
        }
      }
#endif

      /* Assume all RHS symbols can be reached from all symbols of the LHS */
      for (const auto symLHS : lhsSymbols) {
        // XXX do we need to check that LHS must be of type pointer to function or similar?
        for (const auto symRHS : rhsSymbols) {
          if (const auto vDecl = dyn_cast<VarDecl>(symLHS)) {
            aliases[vDecl].insert(symRHS);
            relevantSymbols.insert(vDecl);
          } else {
            std::cerr << "[Warning]: LHS symbol is not of type VarDecl" << std::endl;
          }
        }
      }
    }
    VisitChildren(bo);
  }

  void VisitDeclStmt(DeclStmt *ds) {
    /*
     * We check for declarations of function pointer variables that could then be used inside return statements.
     * We compute path-insensitive target sets of function pointers that a function could return.
     */
    if (ds->isSingleDecl()) {
      if (auto decl = ds->getSingleDecl()) {
        if (auto vDecl = dyn_cast<VarDecl>(decl)) {
          if (vDecl->hasInit()) {
            auto initializer = vDecl->getInit();
            if (auto ice = dyn_cast<ImplicitCastExpr>(initializer)) {
              insertFuncAlias(vDecl, ice);
            } else if (auto innerCe = dyn_cast<CallExpr>(initializer)) {
              RecursiveLookUpCallExpr(innerCe);
            }
          }
        }
      }
    } else {
      // std::cerr << "Decl groups are currently unsupported" << std::endl;
      // exit(-1);
    }
    VisitChildren(ds);
  }
  void VisitReturnStmt(ReturnStmt *RS) {
    if (auto retExpr = RS->getRetValue()) {
      DeclRefRetriever drr(
          [&](DeclRefExpr *dre) {
            const auto isNonCallFuncSym = [](DeclRefExpr *dre) {
              const auto decl = dre->getDecl();
              auto &ctx = decl->getASTContext();
              auto parents = ctx.getParents(*dre);
              auto firstParent = parents.begin();
              if (const auto implCastExpr = dyn_cast<ImplicitCastExpr>((*firstParent).get<Stmt>())) {
                auto implParents = ctx.getParents(*implCastExpr);
                auto potentialCallExpr = implParents.begin();
                if (const auto callExpr = dyn_cast<CallExpr>((*potentialCallExpr).get<Stmt>())) {
                  return false;
                }
              }
              return true;  // This *could* be true
            };
            // Filter semantics: keep if returns false
            return !isNonCallFuncSym(dre);
          },
          relevantSymbols, "FunctionPointerTracer::ReturnStatements");
      drr.Visit(retExpr);
      // drr.printSymbols();
      llvm::SmallSet<const FunctionDecl *, 16> funcSymbols;
      for (const auto sym : drr.getSymbols()) {
        if (const auto fSym = dyn_cast<FunctionDecl>(sym)) {
          funcSymbols.insert(fSym);
        } else if (const auto vSym = dyn_cast<VarDecl>(sym)) {
          for (const auto alias : aliases[vSym]) {
            if (!alias) {
              continue;
            }
            if (const auto fSym = dyn_cast<FunctionDecl>(alias)) {
              funcSymbols.insert(fSym);
            }
          }
        }
      }
      targetsReturn.insert(funcSymbols.begin(), funcSymbols.end());

      if (CallExpr *ce = dyn_cast<CallExpr>(retExpr)) {
        //        RecursiveLookUpCallExpr(ce);
        //        VisitChildren(RS);
        //        return;

        if (closedCes.find(ce) != closedCes.end()) {
          return;  // We already handled this one.
        }
        // XXX Intentionally dead code for now
        const auto dc = ce->getDirectCallee();
        if (!dc) {
          std::cerr << "[Warning] Unable to determine direct callee." << std::endl;
        } else {
          if (auto decl = dyn_cast<FunctionDecl>(dc)) {
            if (auto body = decl->getBody()) {
              {
                std::size_t i = 0;
                for (auto argIter = ce->arg_begin(); argIter != ce->arg_end(); argIter++) {
                  if (i < ce->getDirectCallee()->getNumParams()) {
                    auto paramDecl = decl->getParamDecl(i);
                    if (auto refDecl = (*argIter)->getReferencedDeclOfCallee()) {
                      aliases[paramDecl].insert(refDecl);
                    }
                  }
                  i++;
                }
              }
              FunctionPointerTracer fpt(G, aliases, ce, closedCes);
              fpt.Visit(body);
              auto tr = fpt.getTargetsReturn();
              targetsReturn.insert(tr.begin(), tr.end());
              // std::cout << "Detected more calls. Inserting " << tr.size() << " call targets" << std::endl;
            }
          }
        }
      }

      for (const auto sym : drr.getSymbols()) {
        if (const auto vSym = dyn_cast<VarDecl>(sym)) {
          llvm::SmallSet<const FunctionDecl *, 16> result;
          FindTargetParams(result, vSym, vSym);
          for (const FunctionDecl *decl : result) {
            targetsReturn.insert(decl);
          }
        }
      }
    }
    // XXX Other cases *should* not be of interested to us.
    VisitChildren(RS);
  }
  // FindTargetParams puts all alias FucntionDecls into result
  // To avoid endless circle look ups, (e.g. auto f1 = func; auto f2 = f1; f1 = f2) we use a closed list to track the
  // already processed variables
  void FindTargetParams(llvm::SmallSet<const FunctionDecl *, 16> &result, const Decl *parm, const Decl *cur,
                        llvm::SmallSet<const Decl *, 16> closed = {}) {
    auto alias_iter = aliases.find(cur);
    if (alias_iter != aliases.end()) {
      closed.insert(cur);
      for (auto *new_cur : alias_iter->second) {
        if (auto func_decl = dyn_cast<FunctionDecl>(new_cur)) {
          result.insert(func_decl);
        } else {
          // If new_cur is in closed, we found a circle, we then simply ignore that declartion, elsewise we recursively
          // look up the declartion
          if (closed.find(new_cur) == closed.end()) {
            FindTargetParams(result, parm, new_cur, closed);
          }
        }
      }
    }
  }
  llvm::SmallSet<const FunctionDecl *, 16> target_params(const ParmVarDecl *parm_decl) {
    llvm::SmallSet<const FunctionDecl *, 16> result;
    FindTargetParams(result, parm_decl, parm_decl);
    return result;
  }
  llvm::SmallSet<const FunctionDecl *, 16> getTargetsReturn() const { return targetsReturn; }

 private:
  // add a called decl to another function
  void addCalledDecl(const Decl *caller, const Decl *callee, const CallExpr *C) {
    if (!G->includeInGraph(callee)) {
      return;
    }
    G->getOrInsertNode(caller)->addCallee(G->getOrInsertNode(callee));
    if (C) {
      G->CalledDecls.try_emplace(C, callee);
    }
  }
  auto getAliases() { return aliases; }
  void insertFuncAlias(VarDecl *vDecl, ImplicitCastExpr *ice) {
    auto expr = ice->getSubExpr();
    if (auto dre = dyn_cast<DeclRefExpr>(expr)) {
      insertFuncAlias(vDecl, dre);
    } else {
      // std::cout << "VarDeclInit not a DeclRefExpr" << std::endl;
    }
  }
  void insertFuncAlias(VarDecl *vDecl, DeclRefExpr *dre) {
    if (auto fd = getFunctionDecl(dre)) {
      aliases[vDecl].insert(fd);
      // std::cout << "VisitDeclStmt: " << vDecl->getNameAsString() << " aliases " << fd->getNameAsString() <<
      // std::endl;
    }
  }
  VarDecl *getVarDecl(DeclRefExpr *dre) {
    auto decl = dre->getDecl();
    if (decl && isa<VarDecl>(*decl)) {
      // std::cout << "Found function Reference" << std::endl;
    }
    return dyn_cast<VarDecl>(decl);
  }
  Decl *getFunctionDecl(DeclRefExpr *dre) {
    auto decl = dre->getDecl();
    return decl;
    // if (decl && isa<FunctionDecl>(*decl))
    //{
    //  //std::cout << "Found function Reference" << std::endl;
    //}
    // return dyn_cast<FunctionDecl>(decl);
  }

  llvm::SmallSet<const CallExpr *, 16> closedCes;
  CallGraph *G;
  CallExpr *CE;
  llvm::DenseMap<const Decl *, llvm::SmallSet<const Decl *, 8>> aliases;
  llvm::DenseMap<const VarDecl *, llvm::SmallSet<const FunctionDecl *, 16>> targetParams;
  llvm::SmallSet<const FunctionDecl *, 16> targetsReturn;
  std::unordered_set<const VarDecl *> relevantSymbols;
};

template <class T>
llvm::SmallSet<const FunctionDecl *, 16> getTargetFunctions(CallGraph *g, const T &aliases, CallExpr *ce) {
  llvm::DenseMap<const Decl *, llvm::SmallSet<const Decl *, 8>> someAliases;
  for (const auto &p : aliases) {
    someAliases.insert({p.first, p.second});
  }
  FunctionPointerTracer frf(g, someAliases, ce);
  Decl *de = ce->getDirectCallee();
  if (de) {
    if (Stmt *Body = de->getBody()) {
      frf.Visit(Body);
    }
  }
  return frf.getTargetsReturn();
}

/// A helper class, which walks the AST and locates all the call sites in the
/// given function body.
class CGBuilder : public StmtVisitor<CGBuilder> {
  CallGraph *G;
  CallGraphNode *callerNode;
  std::vector<std::string> vtaInstances;

  bool captureCtorsDtors;
  typedef llvm::DenseMap<const VarDecl *, llvm::SmallSet<const Decl *, 8>> AliasMapT;
  // Stores the aliases per variable
  AliasMapT aliases;
  // We mark functions that have unresolved function symbols left, after we visited them.
  std::unordered_map<const FunctionDecl *, std::unordered_set<const VarDecl *>> unresolvedSymbols;
  std::unordered_set<const VarDecl *> relevantSymbols;

  // Stores the alias sets for variables per function
  std::unordered_map<const FunctionDecl *, AliasMapT> aliasMap;

  std::unordered_map<const VarDecl *, llvm::SmallSet<const FunctionDecl *, 16>> functionPointerTargets;

 public:
  CGBuilder(CallGraph *g, CallGraphNode *N, bool captureCtorsDtors, CallGraph::UnresolvedMapTy unresolvedSyms)
      : G(g), callerNode(N), captureCtorsDtors(captureCtorsDtors), unresolvedSymbols(unresolvedSyms) {}

  void printAliases() const {
    for (const auto &aliasP : aliases) {
      std::cout << "Alias Set for " << aliasP.first->getNameAsString() << " (" << aliasP.first << "\n";
      for (const auto &alias : aliasP.second) {
        std::cout << " |- " << dyn_cast<NamedDecl>(alias)->getNameAsString() << " (" << dyn_cast<NamedDecl>(alias)
                  << ")" << std::endl;
      }
    }
  }

  CallGraph::UnresolvedMapTy getUnresolvedSymbols() { return unresolvedSymbols; }

  void VisitStmt(Stmt *S) { VisitChildren(S); }

  Decl *getDeclFromCall(CallExpr *CE) {
    if (FunctionDecl *CalleeDecl = CE->getDirectCallee()) {
      return CalleeDecl;
    }

    // Simple detection of a call through a block.
    Expr *CEE = CE->getCallee()->IgnoreParenImpCasts();
    if (BlockExpr *Block = dyn_cast<BlockExpr>(CEE)) {
      NumBlockCallEdges++;
      return Block->getBlockDecl();
    }

    return nullptr;
  }

  void handleFunctionPointerInArguments(CallExpr *CE) {
    std::string loc;
    if (CE->getCalleeDecl()) {
      loc = Location(CE->getCalleeDecl(), CE);
    }
    // New
    if (auto directCallee = CE->getDirectCallee()) {
      {  // RAII, so we can declare i again later
        std::size_t i = 0;
        for (auto argIter = CE->arg_begin(); argIter != CE->arg_end(); argIter++) {
          if (i < CE->getDirectCallee()->getNumParams()) {
            auto paramDecl = directCallee->getParamDecl(i);
            // Get the i-th ParameterDecl in the target function
            if (const auto vDecl = dyn_cast<VarDecl>(paramDecl)) {
              // *argIter can be any expression
              const auto argExpr = *argIter;
              DeclRefRetriever drr([]( [[maybe_unused]] DeclRefExpr *dre) { return false; }, relevantSymbols, "Argument Expr Retriever");
              drr.Visit(argExpr);
              // drr.printSymbols();
              for (const auto sym : drr.getSymbols()) {
                aliases[vDecl].insert(sym);
              }
            }
          }
          i++;
        }
      }

      llvm::DenseMap<const Decl *, llvm::SmallSet<const Decl *, 8>> someAliases;
      for (const auto &p : aliases) {
        someAliases.insert({p.first, p.second});
      }
      FunctionPointerTracer fpt(G, someAliases, CE);
      if (auto body = directCallee->getBody()) {
        fpt.Visit(body);
      }
      std::size_t i = 0;
      for (auto argIt = CE->arg_begin(); argIt != CE->arg_end(); ++argIt) {
        VarDecl *varDecl = nullptr;
        Expr *expr = *argIt;
        // Handles out parameters of callee.
        DeclRefRetriever writableFuncPtrVarRetriever(
            [&](DeclRefExpr *dre) {
              const auto isAddrTakenFromVar = [](DeclRefExpr *dre) {
                const auto decl = dre->getDecl();   // required for ASTContext
                auto &ctx = decl->getASTContext();  // required for parent map
                auto parents = ctx.getParents(*dre);
                if (const auto unOp = dyn_cast<UnaryOperator>((*(parents.begin())).get<Stmt>())) {
                  // Wew have found a unary operator
                  const auto compRes = unOp->getOpcode() == UO_AddrOf;
                  return compRes;
                }
                return false;
              };
              // This is used as filter -> filter if return true! We want to keep if addrIsTaken.
              return !isAddrTakenFromVar(dre);
            },
            relevantSymbols, "WritableFuncPtrVarRetriever");
        writableFuncPtrVarRetriever.Visit(expr);
        // writableFuncPtrVarRetriever.printSymbols();

        auto symbols = writableFuncPtrVarRetriever.getSymbols();
        for (const auto sym : symbols) {
          // iterate all referenced symbols
          // If the symbol is a VarDecl of type FunctionPointerType and its address is taken
          // -> Calculate and assign alias set for potential call targets
          if (const auto varSym = dyn_cast<VarDecl>(sym)) {
            varDecl = varSym;
          }
        }

        DeclRefRetriever funcPtrSymRetriever(
            [&](DeclRefExpr *dre) {
              const auto isNonCallFuncSym = [](DeclRefExpr *dre) {
                const auto decl = dre->getDecl();
                auto &ctx = decl->getASTContext();
                auto parents = ctx.getParents(*dre);
                auto firstParent = parents.begin();
                if (const auto implCastExpr = dyn_cast<ImplicitCastExpr>((*firstParent).get<Stmt>())) {
                  auto potentialCallExpr = firstParent++;
                  if (const auto callExpr = dyn_cast<CallExpr>((*potentialCallExpr).get<Stmt>())) {
                    return false;
                  }
                }
                // Keep all that do not match the pattern
                return true;
              };
              // Filter semantics: keep if returns false
              return !isNonCallFuncSym(dre);
            },
            relevantSymbols, "FuncPtrSymRetriever");

        funcPtrSymRetriever.Visit(expr);
        // funcPtrSymRetriever.printSymbols();
        auto funcSymbols = funcPtrSymRetriever.getSymbols();
        for (const auto sym : funcSymbols) {
          // If the symbol is of type FunctionDecl and it is not within a CallExpr and we do not have a callee body
          // -> Assume callee calls the symbol
          if (const auto funSym = dyn_cast<FunctionDecl>(sym)) {
            // Check if the immediate callee has no definition, and if so, add potential call edge
            if (FunctionDecl *calleeDecl = CE->getDirectCallee()) {
              // assumption: callee calls the function which is given in argument list
              if (!calleeDecl->hasBody()) {
                addCalledDecl(calleeDecl, funSym, CE);
              } else {
                // Handle target function.
              }
            }
          } else if (const auto vDecl = dyn_cast<VarDecl>(sym)) {
            // std::cout << "Currently processing symbol" << std::endl;
          }
        }

        if (varDecl) {
          // std::cout << "Found addressed-of VarDecl" << std::endl;
          if (directCallee->getBody()) {
            if (directCallee->getNumParams() > i) {
              ParmVarDecl *parmDecl = directCallee->getParamDecl(i);
              auto targets = fpt.target_params(parmDecl);
              functionPointerTargets[varDecl].insert(targets.begin(), targets.end());
            }
          }
        }
        i++;
      }
    }
  }
  void handleFunctionPointerAsReturn(CallExpr *CE, ASTContext &ctx) {
    // TODO:
    // First, identify which functions can be returned from the call target function
    // Second, identify whether the returned function is called immediately
    // Third, find to which variable it is bound
    // Fourth, find if the variable is called (if so, we already have the information which functions
    //         it could call.
    // Fifth, if the variable is returned... ?
    auto calleeDecl = getDeclFromCall(CE);
    if (!calleeDecl) {
      std::cerr << "Call graph will be incomplete, cannot determine source for function pointers.\n";
    }

    // auto targetFuncSet = getTargetFunctions(calleeDecl);
    auto targetFuncSet = getTargetFunctions(G, aliases, CE);

    const auto isTargetCalledImmediately = [&](CallExpr *ce, ASTContext &c) {
      auto parents = c.getParents(*ce);
      auto pIt = parents.begin();
      auto pStmt = (*pIt).get<Stmt>();
      if (pStmt) {
        return isa<CallExpr>(*pStmt);
      } else {
        return false;
      }
    };

    if (isTargetCalledImmediately(CE, ctx)) {
      // std::cout << "Target is Called immediately" << std::endl;
      for (auto tFunc : targetFuncSet) {
        addCalledDecl(tFunc, CE);
      }
      return;
    }

    const auto getAssignedVar = [&](CallExpr *ce, ASTContext &c) -> const VarDecl * {
      auto parents = c.getParents(*ce);
      auto pIt = parents.begin();
      auto pDecl = (*pIt).get<Decl>();
      if (pDecl) {
        return dyn_cast<VarDecl>(pDecl);
      } else {
        return nullptr;
      }
    };

    const auto dStmt = getAssignedVar(CE, ctx);
    const VarDecl *fVar = dStmt;
    if (fVar) {
      // Store the variable that refers to different functions.
      functionPointerTargets[fVar].insert(targetFuncSet.begin(), targetFuncSet.end());
      return;
    }
  }

  void TraceFunctionPointer(CallExpr *CE) {
    Decl *D = nullptr;
    if ((D = getDeclFromCall(CE))) {
      addCalledDecl(D, CE);
    }
    // If we have CE(foo, 1,2) <- CE gets a function pointer as first argument
    handleFunctionPointerInArguments(CE);

    // If we have foo = CE() <- CE returns a function pointer, then bound to foo
    if (D) {
      auto &ctx = D->getASTContext();
      auto qType = CE->getCallReturnType(ctx);
      if (qType->isFunctionPointerType()) {
        handleFunctionPointerAsReturn(CE, ctx);
      }
    }
  }

  // add a called decl to the current function
  void addCalledDecl(const Decl *D, const CallExpr *C) {
    if (!G->includeInGraph(D))
      return;

    CallGraphNode *CalleeNode = G->getOrInsertNode(D);
    callerNode->addCallee(CalleeNode);
    if (C) {
      G->CalledDecls.try_emplace(C, D);
    }
  }

  // add a called decl to another function
  void addCalledDecl(Decl *Caller, Decl *Callee, CallExpr *C) {
    if (!G->includeInGraph(Callee))
      return;
    G->getOrInsertNode(Caller)->addCallee(G->getOrInsertNode(Callee));
    if (C) {
      G->CalledDecls.try_emplace(C, Callee);
    }
  }

  void VisitCXXConstructExpr(CXXConstructExpr *CE) {
    if (!captureCtorsDtors) {
      return;
    }

    if (auto ctor = CE->getConstructor()) {
      addCalledDecl(ctor, nullptr);
    }

    VisitChildren(CE);
  }

  void VisitCXXDeleteExpr(CXXDeleteExpr *DE) {
    if (!captureCtorsDtors) {
      return;
    }

    auto DT = DE->getDestroyedType();
    if (auto ty = DT.getTypePtrOrNull()) {
      if (!ty->isBuiltinType()) {
        if (auto clDecl = ty->getAsCXXRecordDecl()) {
          if (auto dtor = clDecl->getDestructor()) {
            addCalledDecl(dtor, nullptr);
          }
        }
      }
    }
    VisitChildren(DE);
  }

  void VisitExprWithCleanups(ExprWithCleanups *EWC) {
    [[maybe_unused]] auto nEWC = EWC->getNumObjects();
    auto qty = EWC->getType();
    if (captureCtorsDtors) {
      if (auto ty = qty.getTypePtrOrNull()) {
        if (!ty->isBuiltinType()) {
          if (auto clDecl = ty->getAsCXXRecordDecl()) {
            if (auto dtor = clDecl->getDestructor()) {
              addCalledDecl(dtor, nullptr);
            }
          }
        }
      }
    }
    VisitChildren(EWC);
  }

  void handleCallExprLike(CallExpr *CE) {
    Decl *D = nullptr;
    if ((D = getDeclFromCall(CE))) {
      addCalledDecl(D, CE);
    }

    // If we have CE(foo, 1,2) <- CE gets a function pointer as first argument
    handleFunctionPointerInArguments(CE);

    // If we have foo = CE() <- CE returns a function pointer, then bound to foo
    if (D) {
      auto &ctx = D->getASTContext();
      auto qType = CE->getCallReturnType(ctx);
      bool relevant = qType->isFunctionPointerType() || qType->isFunctionType() || qType->isMemberPointerType();
      if (relevant) {
        handleFunctionPointerAsReturn(CE, ctx);
      }

      std::stringstream ss;
      // If we can determine the call target, we can check for unresolved symbols for that function.
      if (auto func = dyn_cast<FunctionDecl>(D)) {
        ss << "Unresolved before the loop: " << unresolvedSymbols.size() << "\n";
        const int maxUnresolveSteps = 32000;  // XXX: Yes, magic number.
        int numAttempts = 0;
        if (unresolvedSymbols.find(func) != unresolvedSymbols.end()) {
          // Process unresolved symbols
          const auto openSymbols = unresolvedSymbols[func];
          for (const auto sym : openSymbols) {
            std::list<const VarDecl *> worklist;
            std::unordered_set<const VarDecl *> donelist;
            worklist.push_back(sym);

            while (!worklist.empty()) {
              //              std::cout << "worklist size " << worklist.size() << std::endl;
              if (numAttempts > maxUnresolveSteps) {
                ss << "Remaining in worklist: \n";
                for (const auto sym : worklist) {
                  ss << sym->getNameAsString() << "\n";
                }
                std::cout << ss.str() << std::endl;
                break;
              }
              const auto curSym = worklist.front();
              worklist.erase(worklist.begin());
              donelist.insert(curSym);

              for (auto alias : aliases[curSym]) {
                if (const auto vd = dyn_cast<VarDecl>(alias)) {
                  worklist.push_back(vd);
                }
                Decl *nonConstPtr = const_cast<Decl *>(alias);
                addCalledDecl(func, nonConstPtr, CE);
              }
              numAttempts++;
            }
          }
          unresolvedSymbols.erase(unresolvedSymbols.find(func));
          ss << "Unresolved symbols after loop: " << unresolvedSymbols.size() << "\n";
        }
        std::cout << ss.str() << std::endl;
      }
    }
    if (auto ice = CE->getCallee()) {
      DeclRefRetriever drr([]( [[maybe_unused]] DeclRefExpr *dre) { return false; }, relevantSymbols);
      drr.Visit(ice);
#if 0
      std::cout << "CE->getCallee() => drr.Visit(ice)" << std::endl;
      drr.printSymbols();
#endif
      const auto refSymbols = drr.getSymbols();
      for (const auto rSymbol : refSymbols) {
        if (const auto vSymbol = dyn_cast<VarDecl>(rSymbol)) {
          llvm::SmallSet<const FunctionDecl *, 16> localSet;
          for (const auto alias : aliases[vSymbol]) {
            if (const auto funcAlias = dyn_cast<FunctionDecl>(alias)) {
              localSet.insert(funcAlias);
            }
          }
          if (aliases[vSymbol].size() == 0) {
            auto curCallingFunc = dyn_cast<FunctionDecl>(callerNode->getDecl());
            auto &openSymbols = unresolvedSymbols[curCallingFunc];
            openSymbols.insert(vSymbol);
          }
          functionPointerTargets.insert(std::make_pair(vSymbol, localSet));

          const auto targetIter = functionPointerTargets.find(vSymbol);
          if (targetIter != functionPointerTargets.end()) {
            for (const auto func : targetIter->second) {
              addCalledDecl(func, CE);
            }
          }
        } else if (const auto funcSym = dyn_cast<FunctionDecl>(rSymbol)) {
          addCalledDecl(funcSym, CE);
        }
      }
    }

    VisitChildren(CE);
  }

  void VisitCXXMemberCallExpr(CXXMemberCallExpr *mce) { handleCallExprLike(mce); }

  void VisitCallExpr(CallExpr *CE) {
    handleCallExprLike(CE);
    return;
  }

  // Adds may-call edges for the ObjC message sends.
  void VisitObjCMessageExpr(ObjCMessageExpr *ME) {
    if (ObjCInterfaceDecl *IDecl = ME->getReceiverInterface()) {
      Selector Sel = ME->getSelector();

      // Find the callee definition within the same translation unit.
      Decl *D = nullptr;
      if (ME->isInstanceMessage())
        D = IDecl->lookupPrivateMethod(Sel);
      else
        D = IDecl->lookupPrivateClassMethod(Sel);
      if (D) {
        addCalledDecl(D, nullptr);
        NumObjCCallEdges++;
      }
    }
  }

  void VisitChildren(Stmt *S) {
    for (Stmt *SubStmt : S->children())
      if (SubStmt) {
        this->Visit(SubStmt);
      }
  }

  void insertFuncAlias(VarDecl *vDecl, ImplicitCastExpr *ice) {
    auto expr = ice->getSubExpr();
    if (auto dre = dyn_cast<DeclRefExpr>(expr)) {
      insertFuncAlias(vDecl, dre);
    } else {
      // std::cout << "VarDeclInit not a DeclRefExpr" << std::endl;
    }
  }
  FunctionDecl *getFunctionDecl(DeclRefExpr *dre) {
    auto decl = dre->getDecl();
    if (decl && isa<FunctionDecl>(*decl)) {
      // std::cout << "Found function Reference" << std::endl;
    }
    return dyn_cast<FunctionDecl>(decl);
  }
  void insertFuncAlias(VarDecl *vDecl, DeclRefExpr *dre) {
    if (auto fd = getFunctionDecl(dre)) {
      aliases[vDecl].insert(fd);
      // std::cout << "VisitDeclStmt: " << vDecl->getNameAsString() << " aliases " << fd->getNameAsString() <<
      // std::endl;
    }
  }
  VarDecl *getVarDecl(DeclRefExpr *dre) {
    auto decl = dre->getDecl();
    if (decl && isa<VarDecl>(*decl)) {
      // std::cout << "Found function Reference" << std::endl;
    }
    return dyn_cast<VarDecl>(decl);
  }
  void VisitBinaryOperator(BinaryOperator *bo) {
    if (bo->isAssignmentOp()) {
      auto lhs = bo->getLHS();
      auto rhs = bo->getRHS();

      DeclRefRetriever lhsDRR([]( [[maybe_unused]] DeclRefExpr *dre) { return false; }, relevantSymbols);
      lhsDRR.Visit(lhs);
      auto lhsSymbols = lhsDRR.getSymbols();
#if 0
      std::cout << "=== LHS ===" << std::endl;
      for (const auto sym : lhsSymbols) {
        if (const auto nd = dyn_cast<NamedDecl>(sym)) {
          std::cout << "| " << nd->getNameAsString() << std::endl;
        }
      }
#endif

      DeclRefRetriever rhsDRR([]( [[maybe_unused]] DeclRefExpr *dre) { return false; }, relevantSymbols);
      rhsDRR.Visit(rhs);
      auto rhsSymbols = rhsDRR.getSymbols();
#if 0
      std::cout << "=== RHS ===" << std::endl;
      for (const auto sym : rhsSymbols) {
        if (const auto nd = dyn_cast<NamedDecl>(sym)) {
          std::cout << "| " << nd->getNameAsString() << std::endl;
        }
      }
#endif

      /* Assume all RHS symbols can be reached from all symbols of the LHS */
      for (const auto symLHS : lhsSymbols) {
        // XXX do we need to check that LHS must be of type pointer to function or similar?
        for (const auto symRHS : rhsSymbols) {
          if (const auto vDecl = dyn_cast<VarDecl>(symLHS)) {
            aliases[vDecl].insert(symRHS);
            relevantSymbols.insert(vDecl);
          } else {
            std::cerr << "[Warning]: LHS symbol is not of type VarDecl" << std::endl;
          }
        }
      }
    }

    VisitChildren(bo);
  }

  std::unordered_set<Expr *> resolveToDeclRefExpr(ParenExpr *parenExpr) {
    auto subExpr = parenExpr->getSubExpr();
    if (isa<DeclRefExpr>(*subExpr)) {
      return {subExpr};
    }
    if (isa<ConditionalOperator>(*subExpr)) {
      auto potDeclRefs = resolveToDeclRefExpr(dyn_cast<ConditionalOperator>(subExpr));
      if (potDeclRefs.size() > 0) {
        return potDeclRefs;
      }
    }
    return {};
  }
  std::unordered_set<Expr *> resolveToDeclRefExpr(ConditionalOperator *co) {
    auto trueExpr = co->getTrueExpr();
    auto falseExpr = co->getFalseExpr();
    std::unordered_set<Expr *> retSet;
    if (isa<DeclRefExpr>(*trueExpr)) {
      retSet.insert(trueExpr);
    }
    if (isa<DeclRefExpr>(*falseExpr)) {
      retSet.insert(falseExpr);
    }
    return retSet;
  }

  std::unordered_set<Expr *> resolveToDeclRefExpr(CastExpr *castExpr) {
    auto subExpr = castExpr->getSubExpr();
    if (isa<DeclRefExpr>(*subExpr)) {
      return {subExpr};
    }
    // If we try to resolve: check if non-null value is returned, otherwise return null.
    if (dyn_cast<CastExpr>(subExpr)) {
      auto potDeclRefs = resolveToDeclRefExpr(dyn_cast<CastExpr>(subExpr));
      if (potDeclRefs.size() > 0) {
        return potDeclRefs;
      }
    }
    return {};
  }

  void VisitDeclStmt(DeclStmt *ds) {
    /*
     * We check for declarations of function pointer variables that could then be used inside return statements.
     * We compute path-insensitive target sets of function pointers that a function could return.
     */
    if (ds->isSingleDecl()) {
      if (auto rd = dyn_cast<CXXRecordDecl>(ds->getSingleDecl())) {
        vtaInstances.push_back(rd->getQualifiedNameAsString());
      }
      if (auto decl = ds->getSingleDecl()) {
        if (auto vDecl = dyn_cast<VarDecl>(decl)) {
          if (vDecl->hasInit()) {
            auto initializer = vDecl->getInit();
            if (auto ice = dyn_cast<ImplicitCastExpr>(initializer)) {
              insertFuncAlias(vDecl, ice);
            } else if (auto innerCe = dyn_cast<CallExpr>(initializer)) {
              // RecursiveLookUpCallExpr(innerCe);
              (void) innerCe;
            }
          }
        }
      }
    } else {
      // std::cerr << "Decl groups are currently unsupported" << std::endl;
      // exit(-1);
    }
    VisitChildren(ds);
  }
  void VisitCXXNewExpr(CXXNewExpr *new_expr) {
    if (auto *rd = new_expr->getAllocatedType()->getAsCXXRecordDecl()) {
      vtaInstances.push_back(rd->getQualifiedNameAsString());
    }
    VisitChildren(new_expr);
  }
};

void CallGraph::addNodesForBlocks(DeclContext *D) {
  if (BlockDecl *BD = dyn_cast<BlockDecl>(D))
    addNodeForDecl(BD, true);

  for (auto *I : D->decls())
    if (auto *DC = dyn_cast<DeclContext>(I))
      addNodesForBlocks(DC);
}

CallGraph::CallGraph() { Root = getOrInsertNode(nullptr); }

CallGraph::~CallGraph() = default;

bool CallGraph::includeInGraph(const Decl *D) {
  assert(D);

  if (const FunctionDecl *FD = dyn_cast<FunctionDecl>(D)) {
    // We skip function template definitions, as their semantics is
    // only determined when they are instantiated.
    if (FD->isDependentContext())
      return false;

    IdentifierInfo *II = FD->getIdentifier();
    // TODO not sure whether we want to include __inline marked functions
    if (II && II->getName().startswith("__inline")) {
      return true;
    }
  }

  return true;
}

void CallGraph::addNodeForDecl(Decl *D, bool IsGlobal) {
  assert(D);
  (void)IsGlobal;  // silence warning.

  // Allocate a new node, mark it as root, and process it's calls.
  CallGraphNode *Node = getOrInsertNode(D);

  // Process all the calls by this function as well.
  if (Stmt *Body = D->getBody()) {
    CGBuilder builder(this, Node, captureCtorsDtors, unresolvedSymbols);
    builder.Visit(Body);
    // builder.printAliases();
    unresolvedSymbols.insert(builder.getUnresolvedSymbols().begin(), builder.getUnresolvedSymbols().end());
  }
}

CallGraphNode *CallGraph::getNode(const Decl *F) const {
  FunctionMapTy::const_iterator I = FunctionMap.find(F);
  if (I == FunctionMap.end())
    return nullptr;
  return I->second.get();
}

CallGraphNode *CallGraph::getOrInsertNode(const Decl *F) {
  if (F && !isa<ObjCMethodDecl>(F))
    F = F->getCanonicalDecl();

  std::unique_ptr<CallGraphNode> &Node = FunctionMap[F];
  if (Node)
    return Node.get();

  Node = std::make_unique<CallGraphNode>(F);
  // Make Root node a parent of all functions to make sure all are reachable.
  if (F) {
    if (const auto FD = llvm::dyn_cast<FunctionDecl>(F)) {
      inOrderDecls.push_back(FD);
    }
    // std::cout << "Inserting: " << llvm::dyn_cast<NamedDecl>(F)->getNameAsString() << std::endl;
    Root->addCallee(Node.get());
  } else {
    // std::cout << "F was nullptr" << std::endl;
  }
  return Node.get();
}

bool CallGraph::VisitFunctionDecl(clang::FunctionDecl *FD) {
  // We skip function template definitions, as their semantics is
  // only determined when they are instantiated.
  static int count = 0;
  if (includeInGraph(FD) && FD->isThisDeclarationADefinition()) {
    count++;
    if (count % 100 == 0) {
      std::cout << "Processing function nr " << count << std::endl;
    }
    // std::cout << "\n<< Visiting " << FD->getNameAsString() << " >>" << std::endl;
    // Add all blocks declared inside this function to the graph.
    addNodesForBlocks(FD);
    // If this function has external linkage, anything could call it.
    // Note, we are not precise here. For example, the function could have
    // its address taken.
    addNodeForDecl(FD, FD->isGlobal());
  } else {
    // std::cout << "Not including in graph " << FD->getNameAsString() << std::endl;
  }
  return true;
}

bool CallGraph::VisitCXXMethodDecl(clang::CXXMethodDecl *MD) {
  if (!MD->isVirtual()) {
    // std::cout << "Method " << MD->getNameAsString() << " not known to be virtual" << std::endl;
    return true;
  }

  // std::cout << "virtual function " << MD->getParent()->getNameAsString() << "::" << MD->getNameAsString() <<
  // std::endl;

  auto node = getOrInsertNode(MD);

  // multiple inheritance causes multiple entries in overriden_methods
  // hierarchical overridden methods does not show up here
  for (auto om : MD->overridden_methods()) {
    // std::cout << "override " << om->getParent()->getNameAsString() << "::" << om->getNameAsString() << std::endl;

    node->addOverriddenMethod(om);
    getOrInsertNode(om)->addOverriddenBy(node->getDecl());
  }

  return true;
}

void CallGraph::print(raw_ostream &OS) const {
  OS << " --- Call graph Dump --- \n";

  // We are going to print the graph in reverse post order, partially, to make
  // sure the output is deterministic.
  llvm::ReversePostOrderTraversal<const CallGraph *> RPOT(this);
  for (llvm::ReversePostOrderTraversal<const CallGraph *>::rpo_iterator I = RPOT.begin(), E = RPOT.end(); I != E; ++I) {
    const CallGraphNode *N = *I;

    OS << "  Function: ";
    if (N == Root)
      OS << "< root >";
    else
      N->print(OS);

    OS << " calls: ";
    for (CallGraphNode::const_iterator CI = N->begin(), CE = N->end(); CI != CE; ++CI) {
      assert(*CI != Root && "No one can call the root node.");
      (*CI)->print(OS);
      OS << " ";
    }
    OS << '\n';
  }
  OS.flush();
}

LLVM_DUMP_METHOD void CallGraph::dump() const { print(llvm::errs()); }

void CallGraph::viewGraph() const { llvm::ViewGraph(this, "CallGraph"); }

void CallGraphNode::print(raw_ostream &os) const {
  if (const NamedDecl *ND = dyn_cast_or_null<NamedDecl>(FD))
    return ND->printQualifiedName(os);
  os << "< >";
}

LLVM_DUMP_METHOD void CallGraphNode::dump() const { print(llvm::errs()); }

namespace llvm {

template <>
struct DOTGraphTraits<const CallGraph *> : public DefaultDOTGraphTraits {
  DOTGraphTraits(bool isSimple = false) : DefaultDOTGraphTraits(isSimple) {}

  static std::string getNodeLabel(const CallGraphNode *Node, const CallGraph *CG) {
    if (CG->getRoot() == Node) {
      return "< root >";
    }
    if (const NamedDecl *ND = dyn_cast_or_null<NamedDecl>(Node->getDecl()))
      return ND->getNameAsString();
    else
      return "< >";
  }
};

}  // namespace llvm

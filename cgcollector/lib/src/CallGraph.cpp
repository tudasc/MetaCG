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

#include <iostream>

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
    if (auto directCallee = ce->getDirectCallee()) {
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
              addCalledDecl(CE->getCalleeDecl(), fd);
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
      if (auto declRefExpr = dyn_cast<DeclRefExpr>(lhs)) {
        if (auto *vDecl = getVarDecl(declRefExpr)) {
          if (auto ice = dyn_cast<ImplicitCastExpr>(rhs)) {
            insertFuncAlias(vDecl, ice);
          } else if (auto ce = dyn_cast<CallExpr>(rhs)) {
            RecursiveLookUpCallExpr(ce);
          } else {
            // std::cout << "RHS is not an ImplicitCastExpr" << std::endl;
          }
        } else {
          // std::cout << "LHS is not a VarDecl" << std::endl;
        }
      } else if (auto unaryOp = dyn_cast<UnaryOperator>(lhs)) {
        if (auto subExpr = unaryOp->getSubExpr()) {
          if (auto implicitCastExpr = dyn_cast<ImplicitCastExpr>(subExpr)) {
            if (auto subExpr = implicitCastExpr->getSubExpr()) {
              if (auto declRefExpr = dyn_cast<DeclRefExpr>(subExpr)) {
                if (auto vDecl = getVarDecl(declRefExpr)) {
                  if (auto ice = dyn_cast<ImplicitCastExpr>(rhs)) {
                    if (auto *se = ice->getSubExpr()) {
                      if (auto *dre = dyn_cast<DeclRefExpr>(se)) {
                        aliases[vDecl].insert(dre->getDecl());
                      }
                    }
                  } else if (auto innerCe = dyn_cast<CallExpr>(rhs)) {
                    RecursiveLookUpCallExpr(innerCe);
                  } else {
                    // std::cout << "RHS is not an ImplicitCastExpr" << std::endl;
                  }
                } else {
                  // std::cout << "LHS is not a VarDecl" << std::endl;
                }
              }
            }
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
      if (CallExpr *ce = dyn_cast<CallExpr>(retExpr)) {
        if (auto decl = dyn_cast<FunctionDecl>(ce->getDirectCallee())) {
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
      } else if (ImplicitCastExpr *ice = dyn_cast<ImplicitCastExpr>(retExpr)) {
        // This may be what we want
        Expr *cast = ice->getSubExpr();
        if (DeclRefExpr *dre = dyn_cast<DeclRefExpr>(cast)) {
          auto fd = getFunctionDecl(dre);
          if (auto rfd = dyn_cast<FunctionDecl>(fd)) {
            targetsReturn.insert(rfd);
          }
          auto vd = getVarDecl(dre);
          if (vd) {
            llvm::SmallSet<const FunctionDecl *, 16> result;
            FindTargetParams(result, vd, vd);
            for (const FunctionDecl *decl : result) {
              targetsReturn.insert(decl);
            }
            // auto aliases = _aliases[vd];
            // for (auto alias : aliases)
            //{
            //  if (auto func_decl = dyn_cast<FunctionDecl>(alias))
            //  {
            //    _targets_return.insert(func_decl);
            //  }
            //}
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
  void addCalledDecl(const Decl *caller, const Decl *callee) {
    if (!G->includeInGraph(callee)) {
      return;
    }
    G->getOrInsertNode(caller)->addCallee(G->getOrInsertNode(callee));
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
};

template <class T>
llvm::SmallSet<const FunctionDecl *, 16> getTargetFunctions(CallGraph *g, const T &aliases, CallExpr *ce) {
  FunctionPointerTracer frf(g, aliases, ce);
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
  llvm::DenseMap<const Decl *, llvm::SmallSet<const Decl *, 8>> aliases;
  std::vector<std::string> vtaInstances;

  std::unordered_map<const VarDecl *, llvm::SmallSet<const FunctionDecl *, 16>> functionPointerTargets;

  bool captureCtorsDtors;

 public:
  CGBuilder(CallGraph *g, CallGraphNode *N, bool captureCtorsDtors) : G(g), CallerNode(N), captureCtorsDtors(captureCtorsDtors) {}

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
      {
        std::size_t i = 0;
        for (auto argIter = CE->arg_begin(); argIter != CE->arg_end(); argIter++) {
          if (i < CE->getDirectCallee()->getNumParams()) {
            auto paramDecl = directCallee->getParamDecl(i);
            if (auto refDecl = (*argIter)->getReferencedDeclOfCallee()) {
              aliases[paramDecl].insert(refDecl);
            }
          }
          i++;
        }
      }
      FunctionPointerTracer fpt(G, aliases, CE);
      if (auto body = directCallee->getBody()) {
        fpt.Visit(body);
      }
      std::size_t i = 0;
      for (auto argIt = CE->arg_begin(); argIt != CE->arg_end(); ++argIt) {
        VarDecl *varDecl = nullptr;
        // if an adress of operator is used in argument list
        if (UnaryOperator *uo = dyn_cast<UnaryOperator>(*argIt)) {
          if (uo->getOpcode() == UO_AddrOf) {
            if (DeclRefExpr *dre = dyn_cast<DeclRefExpr>(uo->getSubExpr())) {
              // if the adress in the argument list is taken of a function -> function pointer in argument list
              if (FunctionDecl *fd = dyn_cast<FunctionDecl>(dre->getDecl())) {
                // get the called function, which gets the function pointer
                if (FunctionDecl *calleeDecl = CE->getDirectCallee()) {
                  // assumption: callee calls the function which is given in argument list
                  if (!calleeDecl->hasBody()) {
                    addCalledDecl(calleeDecl, fd);
                  }
                  // std::cout << "method " << CalleeDecl->getNameAsString() << " calls " << FD->getNameAsString() << "
                  // by function pointer" << std::endl;

                  // TODO what is done in handle indirects of the old cg? this is a poor implementation we dont want to
                  // follow this here if (CalleeDecl && FD) {
                  //  indirects.insert({CalleeDecl, FD});
                  //}
                }
              } else {
                varDecl = dyn_cast<VarDecl>(dre->getDecl());
                if (varDecl && !varDecl->getType()->isFunctionPointerType()) {
                  varDecl = nullptr;
                }
              }
            }
          }
        } else {
          if (ImplicitCastExpr *ice = llvm::dyn_cast<ImplicitCastExpr>(*argIt)) {
            if (DeclRefExpr *dre = llvm::dyn_cast<DeclRefExpr>(ice->getSubExpr())) {
              // if (auto var_decl = dyn_cast<VarDecl>(dre))
              //{
              //  if (var_decl->getType()->isFunctionPointerType())
              //  {
              //    var_decl = dyn_cast<VarDecl>(dre->getDecl());
              //  }
              //}
              // else
              if (FunctionDecl *fd = llvm::dyn_cast<FunctionDecl>(dre->getDecl())) {
                if (FunctionDecl *cd = CE->getDirectCallee()) {
                  if (!cd->hasBody()) {
                    addCalledDecl(cd, fd);
                  }
                }
              } else {
                varDecl = dyn_cast<VarDecl>(dre->getDecl());
              }
            }
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
        addCalledDecl(tFunc);
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

    std::cout << "Case in which multiple call-chain shit is going on." << std::endl;
  }

  void TraceFunctionPointer(CallExpr *CE) {
    Decl *D = nullptr;
    if ((D = getDeclFromCall(CE))) {
      addCalledDecl(D);
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
  void addCalledDecl(const Decl *D) {
    if (!G->includeInGraph(D))
      return;

    CallGraphNode *CalleeNode = G->getOrInsertNode(D);
    callerNode->addCallee(CalleeNode);
  }

  // add a called decl to another function
  void addCalledDecl(Decl *Caller, Decl *Callee) {
    if (!G->includeInGraph(Callee))
      return;

    G->getOrInsertNode(Caller)->addCallee(G->getOrInsertNode(Callee));
  }

  void VisitCXXConstructExpr(CXXConstructExpr *CE) {
    if (auto ctor = CE->getConstructor()) {
      if (captureCtorsDtors) {
        addCalledDecl(ctor);
      }
    }
  }

  void VisitCallExpr(CallExpr *CE) {
    Decl *D = nullptr;
    if ((D = getDeclFromCall(CE))) {
      addCalledDecl(D);
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
    if (ImplicitCastExpr *ice = dyn_cast<ImplicitCastExpr>(CE->getCallee())) {
      // This may be what we want
      Expr *cast = ice->getSubExpr();
      if (DeclRefExpr *dre = dyn_cast<DeclRefExpr>(cast)) {
        auto *decl = dre->getDecl();
        // if (decl && decl == _currentFunctionPointerVar) {
        if (decl) {
          // std::cout << decl->getNameAsString() << "\n";
          // std::cout << dre->getNameInfo().getAsString() << "\n";
          auto iter = functionPointerTargets.find(reinterpret_cast<const VarDecl *>(decl));
          if (iter != functionPointerTargets.end()) {
            // std::cout << "Found call reference to function pointer" << std::endl;
            for (auto func : iter->second) {
              addCalledDecl(func);
            }
          }
        }
      }
    }

    VisitChildren(CE);
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
        addCalledDecl(D);
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
      if (auto declRefExpr = dyn_cast<DeclRefExpr>(lhs)) {
        if (auto *vDecl = getVarDecl(declRefExpr)) {
          if (auto ice = dyn_cast<ImplicitCastExpr>(rhs)) {
            insertFuncAlias(vDecl, ice);
          } else if (auto ce = dyn_cast<CallExpr>(rhs)) {
            // RecursiveLookUpCallExpr(ce);
          } else {
            // std::cout << "RHS is not an ImplicitCastExpr" << std::endl;
          }
        } else {
          // std::cout << "LHS is not a VarDecl" << std::endl;
        }
      } else if (auto unaryOp = dyn_cast<UnaryOperator>(lhs)) {
        if (auto subExpr = unaryOp->getSubExpr()) {
          if (auto implicitCastExpr = dyn_cast<ImplicitCastExpr>(subExpr)) {
            if (auto subExpr = implicitCastExpr->getSubExpr()) {
              if (auto declRefExpr = dyn_cast<DeclRefExpr>(subExpr)) {
                if (auto vDecl = getVarDecl(declRefExpr)) {
                  if (auto ice = dyn_cast<ImplicitCastExpr>(rhs)) {
                    if (auto *se = ice->getSubExpr()) {
                      if (auto *dre = dyn_cast<DeclRefExpr>(se)) {
                        aliases[vDecl].insert(dre->getDecl());
                      }
                    }
                  } else if (auto innerCe = dyn_cast<CallExpr>(rhs)) {
                    // RecursiveLookUpCallExpr(inner_ce);
                  } else {
                    // std::cout << "RHS is not an ImplicitCastExpr" << std::endl;
                  }
                } else {
                  // std::cout << "LHS is not a VarDecl" << std::endl;
                }
              }
            }
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
  CGBuilder builder(this, Node, captureCtorsDtors);
  if (Stmt *Body = D->getBody())
    builder.Visit(Body);
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
  if (includeInGraph(FD) && FD->isThisDeclarationADefinition()) {
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

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
  assert(D);
#ifndef NDEBUG
  auto fd = llvm::dyn_cast<NamedDecl>(D);
  if (fd) {
    std::cout << fd->getNameAsString() << std::endl;
  }
#endif
}

// Forward
llvm::SmallSet<FunctionDecl *, 16> getTargetFunctions(Decl *FD);

class FunctionReturnsFinder : public StmtVisitor<FunctionReturnsFinder> {
 public:
  void VisitChildren(Stmt *S) {
    for (Stmt *SubStmt : S->children())
      if (SubStmt) {
        this->Visit(SubStmt);
      }
  }
  void VisitStmt(Stmt *S) { VisitChildren(S); }

  void VisitDeclStmt(DeclStmt *DS) {
    /*
     * We check for declarations of function pointer variables that could then be used inside return statements.
     * We compute path-insensitive target sets of function pointers that a function could return.
     */
    if (DS->isSingleDecl()) {
      auto decl = DS->getSingleDecl();
      if (auto vDecl = dyn_cast<VarDecl>(decl)) {
        if (vDecl->hasInit()) {
          auto initializer = vDecl->getInit();
          if (auto ice = dyn_cast<ImplicitCastExpr>(initializer)) {
            insertFuncAlias(vDecl, ice);
          }
        }
      }
    } else {
      std::cerr << "Decl groups are currently unsupported" << std::endl;
      exit(-1);
    }
  }

  void VisitBinaryOperator(BinaryOperator *BO) {
    if (BO->isAssignmentOp()) {
      auto rhs = BO->getRHS();
      auto lhs = BO->getLHS();
      if (auto vDecl = getVarDecl(dyn_cast<DeclRefExpr>(lhs))) {
        if (auto ice = dyn_cast<ImplicitCastExpr>(rhs)) {
          insertFuncAlias(vDecl, ice);
        } else {
          std::cout << "RHS is not an ImplicitCastExpr" << std::endl;
        }
      } else {
        std::cout << "LHS is not a VarDecl" << std::endl;
      }
    }
  }

  void VisitReturnStmt(ReturnStmt *RS) {
    auto retExpr = RS->getRetValue();
    if (CallExpr *ce = dyn_cast<CallExpr>(retExpr)) {
      if (auto decl = dyn_cast<FunctionDecl>(ce->getDirectCallee())) {
        auto transitiveTargets = getTargetFunctions(decl);
        std::cout << "Detected more calls. Inserting " << transitiveTargets.size() << " call targets" << std::endl;
        targets.insert(transitiveTargets.begin(), transitiveTargets.end());
      }
    }

    if (ImplicitCastExpr *ice = dyn_cast<ImplicitCastExpr>(retExpr)) {
      // This may be what we want
      Expr *cast = ice->getSubExpr();
      if (DeclRefExpr *dre = dyn_cast<DeclRefExpr>(cast)) {
        auto fd = getFunctionDecl(dre);
        if (fd) {
          targets.insert(fd);
        }
        auto vd = getVarDecl(dre);
        if (vd) {
          auto aliases = functionAliases[vd];
          targets.insert(aliases.begin(), aliases.end());
        }
      }
    }
    // XXX Other cases *should* not be of interested to us.
  }

  llvm::SmallSet<FunctionDecl *, 16> getTarget() { return targets; }

 private:
  llvm::SmallSet<FunctionDecl *, 16> targets;
  llvm::DenseMap<VarDecl *, llvm::SmallSet<FunctionDecl *, 8>> functionAliases;

  void insertFuncAlias(VarDecl *vDecl, ImplicitCastExpr *ice) {
    auto expr = ice->getSubExpr();
    if (auto dre = dyn_cast<DeclRefExpr>(expr)) {
      insertFuncAlias(vDecl, dre);
    } else {
      std::cout << "VarDeclInit not a DeclRefExpr" << std::endl;
    }
  }

  void insertFuncAlias(VarDecl *vDecl, DeclRefExpr *dre) {
    if (auto fd = getFunctionDecl(dre)) {
      if (functionAliases.find(vDecl) == functionAliases.end()) {
        functionAliases.insert({vDecl, llvm::SmallSet<FunctionDecl *, 8>()});
      }
      functionAliases[vDecl].insert(fd);
      std::cout << "VisitDeclStmt: " << vDecl->getNameAsString() << " aliases " << fd->getNameAsString() << std::endl;
    }
  }

  FunctionDecl *getFunctionDecl(DeclRefExpr *dre) {
    auto decl = dre->getDecl();
    if (decl && isa<FunctionDecl>(*decl)) {
      std::cout << "Found function Reference" << std::endl;
    }
    return dyn_cast<FunctionDecl>(decl);
  }

  VarDecl *getVarDecl(DeclRefExpr *dre) {
    auto decl = dre->getDecl();
    if (decl && isa<VarDecl>(*decl)) {
      std::cout << "Found function Reference" << std::endl;
    }
    return dyn_cast<VarDecl>(decl);
  }
};

llvm::SmallSet<FunctionDecl *, 16> getTargetFunctions(Decl *FD) {
  std::cout << "Running on " << dyn_cast<FunctionDecl>(FD)->getNameAsString() << "\n";
  FunctionReturnsFinder frf;
  if (Stmt *Body = FD->getBody()) {
    frf.Visit(Body);
  }
  return frf.getTarget();
}

/// A helper class, which walks the AST and locates all the call sites in the
/// given function body.
class CGBuilder : public StmtVisitor<CGBuilder> {
  CallGraph *G;
  CallGraphNode *CallerNode;

  const VarDecl *currentFunctionPointerVar = nullptr;
  llvm::SmallSet<FunctionDecl *, 16> functionPointerTargets;

  bool captureCtorsDtors;

 public:
  CGBuilder(CallGraph *g, CallGraphNode *N, bool captureCtorsDtors)
      : G(g), CallerNode(N), captureCtorsDtors(captureCtorsDtors) {}

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
    for (auto argIt = CE->arg_begin(); argIt != CE->arg_end(); ++argIt) {
      // if an adress of operator is used in argument list
      if (UnaryOperator *UO = dyn_cast<UnaryOperator>(*argIt)) {
        if (UO->getOpcode() == UO_AddrOf) {
          // if the adress in the argument list is taken of a function -> function pointer in argument list
          if (DeclRefExpr *DRE = dyn_cast<DeclRefExpr>(UO->getSubExpr())) {
            if (FunctionDecl *FD = dyn_cast<FunctionDecl>(DRE->getDecl())) {
              // get the called function, which gets the function pointer
              if (FunctionDecl *CalleeDecl = CE->getDirectCallee()) {
                // assumption: callee calls the function which is given in argument list
                addCalledDecl(CalleeDecl, FD);
                // std::cout << "method " << CalleeDecl->getNameAsString() << " calls " << FD->getNameAsString() << " by
                // function pointer" << std::endl;

                // TODO what is done in handle indirects of the old cg? this is a poor implementation we dont want to
                // follow this here if (CalleeDecl && FD) {
                //  indirects.insert({CalleeDecl, FD});
                //}
              }
            } else if (VarDecl *varDecl = dyn_cast<VarDecl>(DRE->getDecl())) {
              if (varDecl->getType()->isFunctionPointerType()) {
                std::cout << "Found addressed-of VarDecl" << std::endl;
                // TODO: Determine which functions are assigned to the parameter in the function's definition.
                //       Within a single TU, this can be done the same way as with the return statements.
                //       Across TUs is currently neither supported in this case nor in the Return stmt case.
                //       We have to follow the different symbols along a (potential) call path.
                //       XXX we could actually use the meta information export: Annotate each function that accepts
                //       a pointer to a pointer to a function with the set of functions it assigns to that pointer.
                //       Then the merge tool can construct the edges accordingly.
              }
            }
          }
        }
      } else {
        if (ImplicitCastExpr *ICE = llvm::dyn_cast<ImplicitCastExpr>(*argIt)) {
          if (DeclRefExpr *DRE = llvm::dyn_cast<DeclRefExpr>(ICE->getSubExpr())) {
            if (FunctionDecl *FD = llvm::dyn_cast<FunctionDecl>(DRE->getDecl())) {
              if (FunctionDecl *CalleeDecl = CE->getDirectCallee()) {
                addCalledDecl(CalleeDecl, FD);
              }
            }
          }
        }
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

    auto targetFuncSet = getTargetFunctions(calleeDecl);

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
      std::cout << "Target is Called immediately" << std::endl;
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
      currentFunctionPointerVar = fVar;
      functionPointerTargets = targetFuncSet;
      return;
    }

    std::cout << "Case in which multiple call-chain shit is going on." << std::endl;
  }

  // add a called decl to the current function
  void addCalledDecl(Decl *D) {
    if (!G->includeInGraph(D))
      return;

    CallGraphNode *CalleeNode = G->getOrInsertNode(D);
    CallerNode->addCallee(CalleeNode);
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

    if (currentFunctionPointerVar) {
      if (ImplicitCastExpr *ice = dyn_cast<ImplicitCastExpr>(CE->getCallee())) {
        // This may be what we want
        Expr *cast = ice->getSubExpr();
        if (DeclRefExpr *dre = dyn_cast<DeclRefExpr>(cast)) {
          auto decl = dre->getDecl();
          if (decl && decl == currentFunctionPointerVar) {
            std::cout << "Found call reference to funtion pointer" << std::endl;
            for (auto func : functionPointerTargets) {
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
    std::cout << "Method " << MD->getNameAsString() << " not known to be virtual" << std::endl;
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

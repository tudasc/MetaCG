//===- CallGraph.h - AST-based Call graph -----------------------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
//  This file declares the AST-based CallGraph.
//
//  A call graph for functions whose definitions/bodies are available in the
//  current translation unit. The graph has a "virtual" root node that contains
//  edges to all externally available functions.
//
//===----------------------------------------------------------------------===//

#ifndef CGCOLLECTOR_CALLGRAPHNODEGENERATOR_H
#define CGCOLLECTOR_CALLGRAPHNODEGENERATOR_H

#include <clang/AST/Decl.h>
#include <clang/AST/Expr.h>
#include <clang/AST/RecursiveASTVisitor.h>
#include <llvm/ADT/DenseMap.h>
#include <llvm/ADT/GraphTraits.h>
#include <llvm/ADT/STLExtras.h>
#include <llvm/ADT/SetVector.h>
#include <llvm/ADT/SmallVector.h>

#include <memory>
#include <unordered_map>
#include <unordered_set>

#include "SharedDefs.h"
#include <Callgraph.h>

/// The AST-based call graph.
///
/// The call graph extends itself with the given declarations by implementing
/// the recursive AST visitor, which constructs the graph by visiting the given
/// declarations.

class CallGraphNodeGenerator : public clang::RecursiveASTVisitor<CallGraphNodeGenerator> {
  // We store unresolved symbols across function decl traverses
 public:
  CallGraphNodeGenerator(metacg::Callgraph* cg, bool captureCtorDtor, bool captureNewDelete, bool captureImplicits,
                         bool standalone, AliasAnalysisLevel level)
      : callgraph(cg),
        captureCtorsDtors(captureCtorDtor),
        captureNewDelete(captureNewDelete),
        captureImplicits(captureImplicits),
        standalone(standalone),
        level(level) {};

  CallGraphNodeGenerator() = delete;

  ~CallGraphNodeGenerator() = default;

  /// As we can not get the parent function from a call expr (and we need it, because parent calls child)
  /// we use this to remember the parent function while we traverse the its body
  bool TraverseFunctionDecl(clang::FunctionDecl* D);

  bool TraverseCXXMethodDecl(clang::CXXMethodDecl* MD);

  bool TraverseFunctionTemplateDecl(clang::FunctionTemplateDecl* D);

  bool TraverseClassTemplateDecl(clang::ClassTemplateDecl* D);

  bool TraverseCXXConstructorDecl(clang::CXXConstructorDecl* D);

  // Visitors for Node Generation
  bool VisitFunctionDecl(clang::FunctionDecl* FD);

  /// We need to also visit the Parameter Variable Declerations, as they might be function pointers, which we then need
  /// to add
  bool VisitParmVarDecl(clang::ParmVarDecl* D) { return true; }

  /// The Old Edge Generator
  bool VisitCallExpr(clang::CallExpr* E);

  bool VisitCXXMemberCallExpr(clang::CXXMemberCallExpr* C) { return true; }

  bool VisitCXXConstructExpr(clang::CXXConstructExpr* CE);

  bool VisitCXXDeleteExpr(clang::CXXDeleteExpr* DE);

  // We do not need to visit new separately as constructors capturing ist done via VisitCXXConstructExpr
  bool VisitCXXNewExpr(clang::CXXNewExpr*) { return true; }

  bool shouldVisitImplicitCode() const { return true; }

  bool shouldVisitLambdaBody() const { return true; }

  bool shouldVisitTemplateInstantiations() const { return true; }

  bool shouldWalkTypesOfTypeLocs() const { return false; }

 private:
  void addEdge(clang::Decl* Child);

  void addFunctionPointerAsEntity(clang::Decl* D);

  void addOverestimationData(clang::Decl* D);

  void addNode(const clang::FunctionDecl* D);

  void addPointerMetadataFromPrototype(const clang::FunctionProtoType* protoType);

  /// Determine if a declaration should be included in the graph.
  bool shouldIncludeFunction(const clang::Decl* D);

  metacg::Callgraph* callgraph;
  bool captureCtorsDtors{false};
  bool captureNewDelete{false};
  bool captureImplicits{false};
  bool standalone{false};
  AliasAnalysisLevel level = No;
  std::vector<const clang::Decl*> inOrderDecls;

  /// The Edge Gen Variables
  clang::NamedDecl* topLevelFD = nullptr;

  inline bool ends_with(std::string const& value, std::string const& ending) {
    if (ending.size() > value.size())
      return false;
    return std::equal(ending.rbegin(), ending.rend(), value.rbegin());
  }

  clang::QualType getFinalPointee(clang::QualType pointerType);
};

#endif  // CGCOLLECTOR_CALLGRAPHNODEGENERATOR_H
/**
* File: CallGraphNodeGenerator.h
* License: Part of the MetaCG project. Licensed under BSD 3 clause license. See LICENSE.txt file at
* https://github.com/tudasc/metacg/LICENSE.txt
 */

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
                         bool inferCtorsDtors, bool standalone, AliasAnalysisLevel level)
      : callgraph(cg),
        captureCtorsDtors(captureCtorDtor),
        captureNewDelete(captureNewDelete),
        captureImplicits(captureImplicits),
        inferCtorsDtors(inferCtorsDtors),
        standalone(standalone),
        level(level){};

  CallGraphNodeGenerator() = delete;

  ~CallGraphNodeGenerator() = default;

  /// As we can not get the parent function from a call expr (and we need it, because parent calls child)
  /// we use this to remember the parent function while we traverse the its body
  bool TraverseFunctionDecl(clang::FunctionDecl* D);

  bool TraverseCXXMethodDecl(clang::CXXMethodDecl* MD);

  bool TraverseFunctionTemplateDecl(clang::FunctionTemplateDecl* D);

  bool TraverseClassTemplateDecl(clang::ClassTemplateDecl* D);

  bool TraverseCXXConstructorDecl(clang::CXXConstructorDecl* D);

  bool TraverseCXXDestructorDecl(clang::CXXDestructorDecl* D);

  bool VisitFunctionDecl(clang::FunctionDecl* FD);

  bool VisitCallExpr(clang::CallExpr* E);

  bool VisitCXXDestructorDecl(clang::CXXDestructorDecl* DD);

  bool VisitCXXConstructExpr(clang::CXXConstructExpr* CE);

  bool VisitVarDecl(clang::VarDecl* VD);

  bool VisitCXXDeleteExpr(clang::CXXDeleteExpr* DE);

  bool VisitCXXBindTemporaryExpr(clang::CXXBindTemporaryExpr* CXXBTE);

  bool shouldVisitTemplateInstantiations() const { return true; }

  bool shouldWalkTypesOfTypeLocs() const { return true; }

  bool shouldVisitImplicitCode() const { return true; }

  bool shouldVisitLambdaBody() const { return true; }

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
  bool inferCtorsDtors{false};
  bool standalone{false};
  AliasAnalysisLevel level = No;
  std::unordered_set<void*> traversedTemplates{};

  /// The Edge Gen Variables
  clang::NamedDecl* topLevelFD = nullptr;

  clang::QualType getFinalPointee(clang::QualType pointerType);
};

#endif  // CGCOLLECTOR_CALLGRAPHNODEGENERATOR_H
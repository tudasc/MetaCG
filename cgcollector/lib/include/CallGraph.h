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

#ifndef CGCOLLECTOR_CALLGRAPH_H
#define CGCOLLECTOR_CALLGRAPH_H

#include <clang/AST/Decl.h>
#include <clang/AST/RecursiveASTVisitor.h>
#include <llvm/ADT/DenseMap.h>
#include <llvm/ADT/GraphTraits.h>
#include <llvm/ADT/STLExtras.h>
#include <llvm/ADT/SetVector.h>
#include <llvm/ADT/SmallVector.h>

#include <memory>

class CallGraphNode;

/// The AST-based call graph.
///
/// The call graph extends itself with the given declarations by implementing
/// the recursive AST visitor, which constructs the graph by visiting the given
/// declarations.
class CallGraph : public clang::RecursiveASTVisitor<CallGraph> {
  friend class CallGraphNode;

  using FunctionMapTy = llvm::DenseMap<const clang::Decl *, std::unique_ptr<CallGraphNode>>;

  /// FunctionMap owns all CallGraphNodes.
  FunctionMapTy FunctionMap;

  /// This is a virtual root node that has edges to all the functions.
  CallGraphNode *Root;

  bool captureCtorsDtors{false};

 public:
  CallGraph();
  ~CallGraph();

  void setCaptureCtorsDtors(bool capture) { captureCtorsDtors = capture; }

  /// Populate the call graph with the functions in the given
  /// declaration.
  ///
  /// Recursively walks the declaration to find all the dependent Decls as well.
  void addToCallGraph(clang::Decl *D) { TraverseDecl(D); }

  /// Determine if a declaration should be included in the graph.
  static bool includeInGraph(const clang::Decl *D);

  /// Lookup the node for the given declaration.
  CallGraphNode *getNode(const clang::Decl *) const;

  /// Lookup the node for the given declaration. If none found, insert
  /// one into the graph.
  CallGraphNode *getOrInsertNode(const clang::Decl *);

  using iterator = FunctionMapTy::iterator;
  using const_iterator = FunctionMapTy::const_iterator;

  /// Iterators through all the elements in the graph. Note, this gives
  /// non-deterministic order.
  iterator begin() { return FunctionMap.begin(); }
  iterator end() { return FunctionMap.end(); }
  const_iterator begin() const { return FunctionMap.begin(); }
  const_iterator end() const { return FunctionMap.end(); }

  /// Get the number of nodes in the graph.
  unsigned size() const { return FunctionMap.size(); }

  /// \ brief Get the virtual root of the graph, all the functions available
  /// externally are represented as callees of the node.
  CallGraphNode *getRoot() const { return Root; }

  /// Iterators through all the nodes of the graph that have no parent. These
  /// are the unreachable nodes, which are either unused or are due to us
  /// failing to add a call edge due to the analysis imprecision.
  using nodes_iterator = llvm::SetVector<CallGraphNode *>::iterator;
  using const_nodes_iterator = llvm::SetVector<CallGraphNode *>::const_iterator;

  void print(llvm::raw_ostream &os) const;
  void dump() const;
  void viewGraph() const;

  void addNodesForBlocks(clang::DeclContext *D);

  /// Part of recursive declaration visitation. We recursively visit all the
  /// declarations to collect the root functions.
  bool VisitFunctionDecl(clang::FunctionDecl *FD);
  bool VisitCXXMethodDecl(clang::CXXMethodDecl *MD);

  /// Part of recursive declaration visitation.
  bool VisitObjCMethodDecl(clang::ObjCMethodDecl *MD) {
    if (includeInGraph(MD)) {
      addNodesForBlocks(MD);
      addNodeForDecl(MD, true);
    }
    return true;
  }

  // We are only collecting the declarations, so do not step into the bodies.
  bool TraverseStmt([[maybe_unused]] clang::Stmt *S) { return true; }

  bool shouldWalkTypesOfTypeLocs() const { return false; }
  bool shouldVisitTemplateInstantiations() const { return true; }

 private:
  /// Add the given declaration to the call graph.
  void addNodeForDecl(clang::Decl *D, bool IsGlobal);

  /// Allocate a new node in the graph.
  CallGraphNode *allocateNewNode(clang::Decl *);
};

class CallGraphNode {
 public:
  using CallRecord = CallGraphNode *;

 private:
  /// The function/method declaration.
  const clang::Decl *FD;

  /// The list of functions called from this node.
  llvm::SmallVector<CallRecord, 5> CalledFunctions;

  llvm::SmallVector<const clang::Decl *, 5> OverriddenBy;
  llvm::SmallVector<const clang::Decl *, 5> OverriddenMethods;

  // TODO parents are not handled correctly for virtual functions
  // (virtual functions are not handled correctly in general)
  // this is currently done in pgis
  // so we dont do this for compatibility but we will change this in the future
  llvm::SmallVector<const clang::Decl *, 5> Parents;

 public:
  CallGraphNode(const clang::Decl *D) : FD(D) {}

  using iterator = llvm::SmallVectorImpl<CallRecord>::iterator;
  using const_iterator = llvm::SmallVectorImpl<CallRecord>::const_iterator;

  /// Iterators through all the callees/children of the node.
  iterator begin() { return CalledFunctions.begin(); }
  iterator end() { return CalledFunctions.end(); }
  const_iterator begin() const { return CalledFunctions.begin(); }
  const_iterator end() const { return CalledFunctions.end(); }

  bool empty() const { return CalledFunctions.empty(); }
  unsigned size() const { return CalledFunctions.size(); }

  void addCallee(CallGraphNode *N) {
    CalledFunctions.push_back(N);
    N->Parents.push_back(this->getDecl());
  }

  void addOverriddenBy(const clang::Decl *N) { OverriddenBy.push_back(N); }

  void addOverriddenMethod(const clang::Decl *N) { OverriddenMethods.push_back(N); }
  const llvm::SmallVector<const clang::Decl *, 5> &getOverriddenBy() const { return OverriddenBy; }

  const llvm::SmallVector<const clang::Decl *, 5> &getOverriddenMethods() const { return OverriddenMethods; }

  const llvm::SmallVector<const clang::Decl *, 5> &getParents() const { return Parents; }

  const clang::Decl *getDecl() const { return FD; }

  void print(llvm::raw_ostream &os) const;
  void dump() const;
};

// Graph traits for iteration, viewing.

template <>
struct llvm::GraphTraits<CallGraphNode *> {
  using NodeType = CallGraphNode;
  using NodeRef = CallGraphNode *;
  using ChildIteratorType = NodeType::iterator;

  static NodeType *getEntryNode(CallGraphNode *CGN) { return CGN; }
  static ChildIteratorType child_begin(NodeType *N) { return N->begin(); }
  static ChildIteratorType child_end(NodeType *N) { return N->end(); }
};

template <>
struct llvm::GraphTraits<const CallGraphNode *> {
  using NodeType = const CallGraphNode;
  using NodeRef = const CallGraphNode *;
  using ChildIteratorType = NodeType::const_iterator;

  static NodeType *getEntryNode(const CallGraphNode *CGN) { return CGN; }
  static ChildIteratorType child_begin(NodeType *N) { return N->begin(); }
  static ChildIteratorType child_end(NodeType *N) { return N->end(); }
};

template <>
struct llvm::GraphTraits<CallGraph *> : public llvm::GraphTraits<CallGraphNode *> {
  static NodeType *getEntryNode(CallGraph *CGN) {
    return CGN->getRoot();  // Start at the external node!
  }

  static CallGraphNode *CGGetValue(CallGraph::const_iterator::value_type &P) { return P.second.get(); }

  // nodes_iterator/begin/end - Allow iteration over all nodes in the graph
  using nodes_iterator = mapped_iterator<CallGraph::iterator, decltype(&CGGetValue)>;

  static nodes_iterator nodes_begin(CallGraph *CG) { return nodes_iterator(CG->begin(), &CGGetValue); }

  static nodes_iterator nodes_end(CallGraph *CG) { return nodes_iterator(CG->end(), &CGGetValue); }

  static unsigned size(CallGraph *CG) { return CG->size(); }
};

template <>
struct llvm::GraphTraits<const CallGraph *> : public llvm::GraphTraits<const CallGraphNode *> {
  static NodeType *getEntryNode(const CallGraph *CGN) { return CGN->getRoot(); }

  static CallGraphNode *CGGetValue(CallGraph::const_iterator::value_type &P) { return P.second.get(); }

  // nodes_iterator/begin/end - Allow iteration over all nodes in the graph
  using nodes_iterator = llvm::mapped_iterator<CallGraph::const_iterator, decltype(&CGGetValue)>;

  static nodes_iterator nodes_begin(const CallGraph *CG) { return nodes_iterator(CG->begin(), &CGGetValue); }

  static nodes_iterator nodes_end(const CallGraph *CG) { return nodes_iterator(CG->end(), &CGGetValue); }

  static unsigned size(const CallGraph *CG) { return CG->size(); }
};

#endif  // CGCOLLECTOR_CALLGRAPH_H

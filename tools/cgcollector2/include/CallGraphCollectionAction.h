/**
* File: CallGraphCollectionAction.h
* License: Part of the MetaCG project. Licensed under BSD 3 clause license. See LICENSE.txt file at
* https://github.com/tudasc/metacg/LICENSE.txt
*/

#ifndef CGCOLLECTOR2_CALLGRAPHCOLLECTIONACTION_H
#define CGCOLLECTOR2_CALLGRAPHCOLLECTIONACTION_H

#include "SharedDefs.h"
#include "clang/AST/ASTConsumer.h"
#include "clang/AST/ASTContext.h"
#include "clang/Frontend/FrontendAction.h"

namespace metacg {
class Callgraph;
} // namespace metacg

class CallGraphCollectorConsumer : public clang::ASTConsumer {
 public:
  CallGraphCollectorConsumer(MetaCollectorVector& mcs, int mcgVersion, bool captureCtorsDtors,
                             bool captureNewDeleteCalls, bool captureImplicits, bool inferCtorsDtors, bool prune, bool standalone,
                             AliasAnalysisLevel level)
      : mcs(mcs),
        mcgVersion(mcgVersion),
        captureCtorsDtors(captureCtorsDtors),
        captureNewDeleteCalls(captureNewDeleteCalls),
        captureImplicits(captureImplicits),
        inferCtorsDtors(inferCtorsDtors),
        prune(prune),
        standalone(standalone),
        level(level) {};

  virtual void HandleTranslationUnit(clang::ASTContext& Context);

 private:
  void addOverestimationEdges(metacg::Callgraph* callgraph);

  MetaCollectorVector mcs;
  int mcgVersion;
  bool captureCtorsDtors;
  bool captureNewDeleteCalls;
  bool captureImplicits;
  bool inferCtorsDtors;
  bool prune;
  bool standalone;
  AliasAnalysisLevel level;
};

class CallGraphCollectorAction : clang::ASTFrontendAction {
 public:
  CallGraphCollectorAction(MetaCollectorVector& mcs, int mcgVersion, bool captureCtorsDtors, bool captureNewDeleteCalls,
                           bool captureImplicits, bool infereCtorsDtors, bool prune, bool standalone, AliasAnalysisLevel level)
      : mcs(mcs),
        mcgVersion(mcgVersion),
        captureCtorsDtors(captureCtorsDtors),
        captureNewDeleteCalls(captureNewDeleteCalls),
        captureImplicits(captureImplicits),
        inferCtorsDtors(infereCtorsDtors),
        prune(prune),
        standalone(standalone),
        level(level) {}

  std::unique_ptr<clang::ASTConsumer> newASTConsumer() {
    return std::unique_ptr<clang::ASTConsumer>(new CallGraphCollectorConsumer(
        mcs, mcgVersion, captureCtorsDtors, captureNewDeleteCalls, captureImplicits, inferCtorsDtors, prune, standalone, level));
  }

  std::unique_ptr<clang::ASTConsumer> CreateASTConsumer([[maybe_unused]] clang::CompilerInstance& compiler,
                                                        [[maybe_unused]] llvm::StringRef sr) {
    return std::unique_ptr<clang::ASTConsumer>(new CallGraphCollectorConsumer(
        mcs, mcgVersion, captureCtorsDtors, captureNewDeleteCalls, captureImplicits, inferCtorsDtors, prune, standalone, level));
  }

 private:
  MetaCollectorVector mcs;
  int mcgVersion;
  bool captureCtorsDtors;
  bool captureNewDeleteCalls;
  bool captureImplicits;
  bool inferCtorsDtors;
  bool prune;
  bool standalone;
  AliasAnalysisLevel level;
};

#endif  // CGCOLLECTOR2_CALLGRAPHCOLLECTIONACTION_H

/**
* File: GlobalLoopDepthCollector.h
* License: Part of the MetaCG project. Licensed under BSD 3 clause license. See LICENSE.txt file at
* https://github.com/tudasc/metacg/LICENSE.txt
*/

#ifndef CGCOLLECTOR2_GLOBALLOOPDEPTHCOLLECTOR_H
#define CGCOLLECTOR2_GLOBALLOOPDEPTHCOLLECTOR_H

#include "ASTUtil.h"
#include "metadata/LoopMD.h"

#include <iostream>

struct GlobalLoopDepthCollector final : public Plugin {
 public:
  GlobalLoopDepthMD* computeForDecl(const clang::FunctionDecl* const functionDecl) override { return nullptr; }

  void computeForGraph(const metacg::Callgraph* const cg) override {}

  std::string getPluginName() override { return "GlobalLoopDepthCollector"; }

 private:
};

#endif  // CGCOLLECTOR2_GLOBALLOOPDEPTHCOLLECTOR_H

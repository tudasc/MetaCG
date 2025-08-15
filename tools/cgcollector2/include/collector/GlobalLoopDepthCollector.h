/**
* File: GlobalLoopDepthCollector.h
* License: Part of the MetaCG project. Licensed under BSD 3 clause license. See LICENSE.txt file at
* https://github.com/tudasc/metacg/LICENSE.txt
*/

#ifndef CGCOLLECTOR2_GLOBALLOOPDEPTHCOLLECTOR_H
#define CGCOLLECTOR2_GLOBALLOOPDEPTHCOLLECTOR_H

#include "metadata/LoopMD.h"

struct GlobalLoopDepthCollector final : public Plugin {
 public:
  std::unique_ptr<metacg::MetaData> computeForDecl(const clang::FunctionDecl* const) override { return nullptr; }

  void computeForGraph(const metacg::Callgraph* const) override {}

  std::string getPluginName() override { return "GlobalLoopDepthCollector"; }

 private:
};

#endif  // CGCOLLECTOR2_GLOBALLOOPDEPTHCOLLECTOR_H

/**
 * File: CallGraphEmbedder.h
 * License: Part of the MetaCG project. Licensed under BSD 3 clause license. See LICENSE.txt file at
 * https://github.com/tudasc/metacg/LICENSE.txt
 */
#ifndef METACG_CALLGRAPHEMBEDDER_H
#define METACG_CALLGRAPHEMBEDDER_H

#include "cage/interface/CaGePlugin.h"

#include <string>

namespace llvm {
class Module;
}

namespace cage {

class GraphEmbedder : public Plugin {
 public:
  explicit GraphEmbedder(llvm::Module& M) : GraphEmbedder(M, "metacg") {}
  GraphEmbedder(llvm::Module& M, const std::string& sectionName) : M(M), sectionName(sectionName) {}
  void consumeCallGraph(const metacg::Callgraph&) override;

 private:
  llvm::Module& M;
  std::string sectionName;
};

}  // namespace cage

#endif  // METACG_CALLGRAPHEMBEDDER_H

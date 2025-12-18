/**
* File: MetaCollector.h
* License: Part of the MetaCG project. Licensed under BSD 3 clause license. See LICENSE.txt file at
* https://github.com/tudasc/metacg/LICENSE.txt
*/
#ifndef METACG_METACOLLECTOR_H
#define METACG_METACOLLECTOR_H

#include "llvm/IR/Module.h"
#include "Callgraph.h"

namespace cage {

class MetaCollector {
 public:
  virtual void run(llvm::Module&, metacg::Callgraph&) = 0;
};

class FunctionLocalMetaCollector: public MetaCollector {
 public:
  virtual std::unique_ptr<metacg::MetaData> runOnFunction(llvm::Function&) = 0;

  void run(llvm::Module& M, metacg::Callgraph& cg) override {
    for (auto& F: M) {
      auto node = cg.getFirstNode(F.getName().str());
      if (!node) {
        continue;
      }
      if (auto md = runOnFunction(F)) {
        node->addMetaData(std::move(md));
      }
    }
  }
};

}

#endif  // METACG_METACOLLECTOR_H

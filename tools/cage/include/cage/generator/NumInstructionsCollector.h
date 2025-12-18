/**
* File: NumInstructionsCollector.h
* License: Part of the MetaCG project. Licensed under BSD 3 clause license. See LICENSE.txt file at
* https://github.com/tudasc/metacg/LICENSE.txt
 */
 
#ifndef METACG_NUMINSTRUCTIONSCOLLECTOR_H
#define METACG_NUMINSTRUCTIONSCOLLECTOR_H

#include "MetaCollector.h"
#include "NumInstructionsMD.h"

namespace cage {

class NumInstructionsCollector: public FunctionLocalMetaCollector {
 public:
  std::unique_ptr<metacg::MetaData> runOnFunction(llvm::Function& F) override {
    return std::make_unique<NumInstructionsMD>(F.getInstructionCount());
  }
};

}
#endif 
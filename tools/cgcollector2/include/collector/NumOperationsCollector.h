/**
* File: NumOperationsCollector.h
* License: Part of the MetaCG project. Licensed under BSD 3 clause license. See LICENSE.txt file at
* https://github.com/tudasc/metacg/LICENSE.txt
*/

#ifndef CGCOLLECTOR2_NUMOPERATIONSCOLLECTOR_H
#define CGCOLLECTOR2_NUMOPERATIONSCOLLECTOR_H

#include "Plugin.h"
#include "metadata/NumOperationsMD.h"

struct NumOperationsCollector : public Plugin {
  virtual NumOperationsMD* computeForDecl(clang::FunctionDecl const* const decl) {
    auto result = new NumOperationsMD;
    const auto counts = getNumOperationsInStmt(decl->getBody());
    result->numberOfIntOps = counts.numberOfIntOps;
    result->numberOfFloatOps = counts.numberOfFloatOps;
    result->numberOfControlFlowOps = counts.numberOfControlFlowOps;
    result->numberOfMemoryAccesses = counts.numberOfMemoryAccesses;
    return result;
  }

  std::string getPluginName() override { return "NumOperationsCollector"; }

  virtual ~NumOperationsCollector() = default;
};

#endif  // CGCOLLECTOR2_NUMOPERATIONSCOLLECTOR_H

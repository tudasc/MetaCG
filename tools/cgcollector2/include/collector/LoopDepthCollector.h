/**
* File: LoopDepthCollector.h
* License: Part of the MetaCG project. Licensed under BSD 3 clause license. See LICENSE.txt file at
* https://github.com/tudasc/metacg/LICENSE.txt
*/

#ifndef CGCOLLECTOR2_LOOPDEPTHCOLLECTOR_H
#define CGCOLLECTOR2_LOOPDEPTHCOLLECTOR_H

#include "MetaDataFunctions.h"
#include "Plugin.h"
#include "metadata/LoopMD.h"

struct LoopDepthCollector : public Plugin {
  virtual  std::unique_ptr<metacg::MetaData> computeForDecl(clang::FunctionDecl const* const decl) {
    std::unique_ptr<metacg::LoopDepthMD> result = std::make_unique<metacg::LoopDepthMD>();
    result->loopDepth = getLoopDepthInStmt(decl->getBody());
    return result;
  }

  virtual std::string getPluginName() { return "LoopDepthCollector"; }

 public:
  virtual ~LoopDepthCollector() = default;
};

#endif  // CGCOLLECTOR2_LOOPDEPTHCOLLECTOR_H

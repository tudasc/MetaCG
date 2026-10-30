/**
 * File: LoopDepthCollector.h
 * License: Part of the MetaCG project. Licensed under BSD 3 clause license. See LICENSE.txt file at
 * https://github.com/tudasc/metacg/LICENSE.txt
 */

#ifndef CGCOLLECTOR2_LOOPDEPTHCOLLECTOR_H
#define CGCOLLECTOR2_LOOPDEPTHCOLLECTOR_H

#include "cgcollector2/interface/CGC2Plugin.h"
#include "metacg/metadata/LoopMD.h"


struct LoopDepthCollector : public cgcollector2::Plugin {
  std::unique_ptr<metacg::MetaData> computeForDecl(clang::FunctionDecl const* const decl) override ;

  std::string getPluginName() const final { return "LoopDepthCollector"; }

  virtual ~LoopDepthCollector() = default;
};

#endif  // CGCOLLECTOR2_LOOPDEPTHCOLLECTOR_H

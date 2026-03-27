/**
* File: NumConditionalBranchCollector.h
* License: Part of the MetaCG project. Licensed under BSD 3 clause license. See LICENSE.txt file at
* https://github.com/tudasc/metacg/LICENSE.txt
*/

#ifndef CGCOLLECTOR2_NUMCONDITIONALBRANCHCOLLECTOR_H
#define CGCOLLECTOR2_NUMCONDITIONALBRANCHCOLLECTOR_H

#include "metacg/metadata/NumConditionalBranchMD.h"
#include "cgcollector2/interface/CGC2Plugin.h"

struct NumConditionalBranchCollector : public cgcollector2::Plugin {
  virtual std::unique_ptr<metacg::MetaData> computeForDecl(clang::FunctionDecl const* const decl) {
    std::unique_ptr<metacg::NumConditionalBranchMD> result = std::make_unique<metacg::NumConditionalBranchMD>();
    result->numConditionalBranches = getNumConditionalBranchesInStmt(decl->getBody());
    return result;
  }

  std::string getPluginName() const final { return "NumConditionalBranchCollector"; }

  virtual ~NumConditionalBranchCollector() = default;
};

#endif  // CGCOLLECTOR2_NUMCONDITIONALBRANCHCOLLECTOR_H

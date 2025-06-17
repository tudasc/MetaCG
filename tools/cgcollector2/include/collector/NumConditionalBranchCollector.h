/**
* File: NumConditionalBranchCollector.h
* License: Part of the MetaCG project. Licensed under BSD 3 clause license. See LICENSE.txt file at
* https://github.com/tudasc/metacg/LICENSE.txt
*/

#ifndef CGCOLLECTOR2_NUMCONDITIONALBRANCHCOLLECTOR_H
#define CGCOLLECTOR2_NUMCONDITIONALBRANCHCOLLECTOR_H

#include "Plugin.h"
#include "metadata/NumConditionalBranchMD.h"

struct NumConditionalBranchCollector : public Plugin {
  virtual NumConditionalBranchMD* computeForDecl(clang::FunctionDecl const* const decl) {
    auto result = new NumConditionalBranchMD();
    result->numConditionalBranches = getNumConditionalBranchesInStmt(decl->getBody());
    return result;
  }

  virtual std::string getPluginName() { return "NumConditionalBranchCollector"; }

 public:
  virtual ~NumConditionalBranchCollector() = default;
};

#endif  // CGCOLLECTOR2_NUMCONDITIONALBRANCHCOLLECTOR_H

/**
* File: CodeStatisticsCollector.h
* License: Part of the MetaCG project. Licensed under BSD 3 clause license. See LICENSE.txt file at
* https://github.com/tudasc/metacg/LICENSE.txt
*/
#ifndef CGCOLLECTOR2_CODESTATISTICSCOLLECTOR_H
#define CGCOLLECTOR2_CODESTATISTICSCOLLECTOR_H

#include "Plugin.h"
#include "metadata/CodeStatisticsMD.h"

#include <clang/AST/Decl.h>

struct CodeStatisticsCollector : public Plugin {
  virtual std::unique_ptr<metacg::MetaData> computeForDecl(clang::FunctionDecl const* const decl) {
    std::unique_ptr<metacg::CodeStatisticsMD> result = std::make_unique<metacg::CodeStatisticsMD>();
    for (auto declIter = decl->decls_begin(); declIter != decl->decls_end(); ++declIter) {
      if (const auto varDecl = llvm::dyn_cast<clang::VarDecl>(*declIter)) {
        result->numVars++;
      }
    }
    return result;
  }

  virtual std::string getPluginName() { return "CodeStatisticsCollector"; }

 public:
  virtual ~CodeStatisticsCollector() = default;
};
#endif  // CGCOLLECTOR2_CODESTATISTICSCOLLECTOR_H

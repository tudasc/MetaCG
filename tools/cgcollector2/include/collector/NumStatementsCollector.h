/**
* File: NumStatementsCollector.h
* License: Part of the MetaCG project. Licensed under BSD 3 clause license. See LICENSE.txt file at
* https://github.com/tudasc/metacg/LICENSE.txt
 */
#ifndef CGCOLLECTOR2_NUMSTATEMENTSCOLLECTOR_H
#define CGCOLLECTOR2_NUMSTATEMENTSCOLLECTOR_H

#include "Callgraph.h"

#include "Plugin.h"

#include "MetaDataFunctions.h"
#include "metadata/NumStatementsMD.h"

#include <clang/AST/Decl.h>

struct NumberOfStatementsCollector : public Plugin {
  std::unique_ptr<metacg::MetaData> computeForDecl(const clang::FunctionDecl* const functionDecl) override {
    std::unique_ptr<metacg::NumStatementsMD> result = std::make_unique<metacg::NumStatementsMD>();
    result->setNumberOfStatements(getNumStmtsInStmt(functionDecl->getBody()));
    return result;
  }

  void computeForGraph(const metacg::Callgraph* const) override {}

  virtual std::string getPluginName() override { return "NumberOfStatementsCollector"; }
};

#endif  // CGCOLLECTOR2_NUMSTATEMENTSCOLLECTOR_H

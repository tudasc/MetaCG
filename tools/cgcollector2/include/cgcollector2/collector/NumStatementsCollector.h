/**
 * File: NumStatementsCollector.h
 * License: Part of the MetaCG project. Licensed under BSD 3 clause license. See LICENSE.txt file at
 * https://github.com/tudasc/metacg/LICENSE.txt
 */
#ifndef CGCOLLECTOR2_NUMSTATEMENTSCOLLECTOR_H
#define CGCOLLECTOR2_NUMSTATEMENTSCOLLECTOR_H

#include "cgcollector2/interface/CGC2Plugin.h"
#include "metacg/Callgraph.h"
#include "metacg/metadata/NumStatementsMD.h"


struct NumberOfStatementsCollector : public cgcollector2::Plugin {
  std::unique_ptr<metacg::MetaData> computeForDecl(const clang::FunctionDecl* const functionDecl) override;

  void computeForGraph(metacg::Callgraph* const) override {}

  std::string getPluginName() const final { return "NumberOfStatementsCollector"; }
};

#endif  // CGCOLLECTOR2_NUMSTATEMENTSCOLLECTOR_H

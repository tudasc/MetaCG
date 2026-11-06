/**
 * File: StatementCountCollector.cpp
 * License: Part of the MetaCG project. Licensed under BSD 3 clause license. See LICENSE.txt file at
 * https://github.com/tudasc/metacg/LICENSE.txt
 */

#include "StatementCountCollector.h"
#include "metacg/metadata/NumStatementsMD.h"

#include <flang/Parser/parse-tree.h>
#include <flang/Semantics/symbol.h>

bool StatementCountVisitor::Pre(const Fortran::parser::MainProgram& p) {
  inProcedure = true;
  statementCounts.push_back(0);
  return true;
}

bool StatementCountVisitor::Pre(const Fortran::parser::FunctionSubprogram&) {
  inProcedure = true;
  statementCounts.push_back(0);
  return true;
}

bool StatementCountVisitor::Pre(const Fortran::parser::SubroutineSubprogram&) {
  inProcedure = true;
  statementCounts.push_back(0);
  return true;
}

void StatementCountVisitor::Post(const Fortran::parser::MainProgram& p) {
  auto& stmt = std::get<std::optional<Fortran::parser::Statement<Fortran::parser::ProgramStmt>>>(p.t);
  if (!stmt.has_value()) {
    return;
  }
  auto symbol = stmt.value().statement.v.symbol;
  auto mangledName = metacg::cgfcollector::mangleSymbol(symbol, underscoring);
  addStatementCountToNode(mangledName);

  inProcedure = false;
  statementCounts.pop_back();
}

void StatementCountVisitor::Post(const Fortran::parser::FunctionSubprogram& p) {
  auto& stmt = std::get<Fortran::parser::Statement<Fortran::parser::FunctionStmt>>(p.t);
  auto* symbol = std::get<Fortran::parser::Name>(stmt.statement.t).symbol;
  auto mangledName = metacg::cgfcollector::mangleSymbol(symbol, underscoring);
  addStatementCountToNode(mangledName);

  inProcedure = false;
  statementCounts.pop_back();
}

void StatementCountVisitor::Post(const Fortran::parser::SubroutineSubprogram& p) {
  auto& stmt = std::get<Fortran::parser::Statement<Fortran::parser::SubroutineStmt>>(p.t);
  auto* symbol = std::get<Fortran::parser::Name>(stmt.statement.t).symbol;
  auto mangledName = metacg::cgfcollector::mangleSymbol(symbol, underscoring);
  addStatementCountToNode(mangledName);

  inProcedure = false;
  statementCounts.pop_back();
}

void StatementCountVisitor::addStatementCountToNode(const std::string& mangledName) {
  auto* node = cg->getFirstNode(mangledName);
  if (!node) {
    return;
  }
  auto numStmt = std::make_unique<metacg::NumStatementsMD>();
  numStmt->setNumberOfStatements(statementCounts.back());
  node->addMetaData(std::move(numStmt));
}

void StatementCountVisitor::incrementCount() {
  if (inProcedure) {
    statementCounts.back()++;
  }
}

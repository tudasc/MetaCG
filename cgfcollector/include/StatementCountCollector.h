/**
 * File: StatementCountCollector.h
 * License: Part of the MetaCG project. Licensed under BSD 3 clause license. See LICENSE.txt file at
 * https://github.com/tudasc/metacg/LICENSE.txt
 */

#ifndef STATEMENTCOUNTCOLLECTOR_H
#define STATEMENTCOUNTCOLLECTOR_H

#include "FortranUtil.h"

#include "metacg/Callgraph.h"
#include "metacg/io/IdMapping.h"
#include <cstddef>
#include <flang/Parser/parse-tree.h>
#include <optional>

class StatementCountMetadata : public metacg::MetaData::Registrar<StatementCountMetadata> {
 public:
  static constexpr const char* key = "statement_count";

  StatementCountMetadata(std::size_t count) : count(count) {}
  explicit StatementCountMetadata(const nlohmann::json& j, metacg::StrToNodeMapping&);

  nlohmann::json toJson(metacg::NodeToStrMapping&) const final { return count; }

  const char* getKey() const override { return key; }

  void merge(const MetaData& toMerge, std::optional<metacg::MergeAction>, const metacg::GraphMapping&) final;

  std::unique_ptr<MetaData> clone() const final { return std::unique_ptr<MetaData>(new StatementCountMetadata(*this)); }

  void applyMapping(const metacg::GraphMapping&) override {}

  void setCount(std::size_t c) { count = c; }
  std::size_t getCount() const { return count; }

 private:
  std::size_t count{0};
};

class StatementCountVisitor {
 public:
  StatementCountVisitor(metacg::Callgraph* cg, bool underscoring) : cg(cg), underscoring(underscoring) {}

  template <typename A>
  bool Pre(const A&) {
    return true;
  }
  template <typename A>
  void Post(const A&) {}

  bool Pre(const Fortran::parser::MainProgram& p);
  bool Pre(const Fortran::parser::FunctionSubprogram&);
  bool Pre(const Fortran::parser::SubroutineSubprogram&);
  void Post(const Fortran::parser::MainProgram& p);
  void Post(const Fortran::parser::FunctionSubprogram& p);
  void Post(const Fortran::parser::SubroutineSubprogram& p);

  void addStatementCountToNode(const std::string& mangledName);
  void incrementCount();

  // Count statements in the procedure body

#define COUNT_STATEMENT(stmtType)                                                                                      \
  bool Pre(const Fortran::parser::stmtType&) {                                                                         \
    incrementCount();                                                                                                  \
    return true;                                                                                                       \
  }

  COUNT_STATEMENT(ImplicitStmt)
  COUNT_STATEMENT(DeclarationConstruct)
  COUNT_STATEMENT(ExecutionPartConstruct)

  COUNT_STATEMENT(WhereBodyConstruct)
  COUNT_STATEMENT(ForallBodyConstruct)

 private:
  bool inProcedure{false};
  bool underscoring;
  metacg::Callgraph* cg;
  std::vector<std::size_t> statementCounts;
};

#endif  // STATEMENTCOUNTCOLLECTOR_H

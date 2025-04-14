/**
 * File: NumStatementsMD.h
 * License: Part of the MetaCG project. Licensed under BSD 3 clause license. See LICENSE.txt file at
 * https://github.com/tudasc/metacg/LICENSE.txt
 */
#ifndef CGCOLLECTOR2_NUMSTATEMENTSMD_H
#define CGCOLLECTOR2_NUMSTATEMENTSMD_H

#include "metadata/MetaData.h"

class NumStatementsMD : public metacg::MetaData::Registrar<NumStatementsMD> {
 public:
  static constexpr const char* key = "numStatements";
  NumStatementsMD() = default;
  explicit NumStatementsMD(const nlohmann::json& j) {
    metacg::MCGLogger::instance().getConsole()->trace("Reading NumStatementsMD from json");
    if (j.is_null()) {
      metacg::MCGLogger::instance().getConsole()->trace("Could not retrieve meta data for {}", "NumStatementsMD");
      return;
    }
    auto jsonNumStmts = j.get<long long int>();
    metacg::MCGLogger::instance().getConsole()->debug("Read {} stmts from file", jsonNumStmts);
    setNumberOfStatements(jsonNumStmts);
  }

 private:
  NumStatementsMD(const NumStatementsMD& other) : numStmts(other.numStmts) {}

 public:
  nlohmann::json to_json() const final { return getNumberOfStatements(); }

  const char* getKey() const override { return key; }

  void merge(const MetaData& toMerge) final {
    assert(toMerge.getKey() == getKey() && "Trying to merge NumStatementsMD with meta data of different types");

    const NumStatementsMD* toMergeDerived = static_cast<const NumStatementsMD*>(&toMerge);

    int a = toMergeDerived->getNumberOfStatements();
    int b = numStmts;

    if (numStmts != 0 && toMergeDerived->getNumberOfStatements() != 0 &&
        numStmts != toMergeDerived->getNumberOfStatements()) {
      metacg::MCGLogger::instance().getErrConsole()->warn(
          "Same function defined with different number of statements found on merge.");
    }
    numStmts = std::max(numStmts, toMergeDerived->getNumberOfStatements());
  }

  MetaData* clone() const final { return new NumStatementsMD(*this); }

  void setNumberOfStatements(int numStmts) { this->numStmts = numStmts; }
  int getNumberOfStatements() const { return this->numStmts; }

 private:
  int numStmts{0};
};

#endif  // CGCOLLECTOR2_NUMSTATEMENTSMD_H

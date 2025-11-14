/**
* File: CodeStatisticsMD.h
* License: Part of the MetaCG project. Licensed under BSD 3 clause license. See LICENSE.txt file at
* https://github.com/tudasc/metacg/LICENSE.txt
*/
#ifndef CGCOLLECTOR2_CODESTATISTICSMD_H
#define CGCOLLECTOR2_CODESTATISTICSMD_H

#include "metadata/MetaData.h"

namespace metacg {

class CodeStatisticsMD : public metacg::MetaData::Registrar<CodeStatisticsMD> {
 public:
  static constexpr const char* key = "codeStatistics";
  CodeStatisticsMD() = default;
  explicit CodeStatisticsMD(const nlohmann::json& j, StrToNodeMapping&) {
    if (j.is_null()) {
      metacg::MCGLogger::instance().getConsole()->error("Could not retrieve meta data for {}", key);
      return;
    }
    int jNumVars = j["numVars"].get<int>();
    numVars = jNumVars;
  }

 private:
  CodeStatisticsMD(const CodeStatisticsMD& other) : numVars(other.numVars) {}

 public:
  nlohmann::json toJson(NodeToStrMapping&) const final {
    nlohmann::json j;
    j["numVars"] = numVars;
    return j;
  }

  const char* getKey() const final { return key; }

  void merge(const MetaData& toMerge, std::optional<MergeAction>, const GraphMapping&) final {
    assert(toMerge.getKey() == getKey() && "Trying to merge CodeStatisticsMD with meta data of different types");

    const CodeStatisticsMD* toMergeDerived = static_cast<const CodeStatisticsMD*>(&toMerge);

    if (numVars != toMergeDerived->numVars) {
      numVars += toMergeDerived->numVars;

      if (numVars != 0 && toMergeDerived->numVars != 0) {
        metacg::MCGLogger::instance().getErrConsole()->warn(
            "Same function defined with different number of variables found on merge.");
      }
    }
  }

  std::unique_ptr<MetaData> clone() const final { return std::unique_ptr<MetaData>(new CodeStatisticsMD(*this)); }

  void applyMapping(const GraphMapping&) override {}

  int numVars{0};
};

}  // namespace metacg

#endif  // CGCOLLECTOR2_CODESTATISTICSMD_H

/**
 * File: NumOperationsMD.h
 * License: Part of the MetaCG project. Licensed under BSD 3 clause license. See LICENSE.txt file at
 * https://github.com/tudasc/metacg/LICENSE.txt
 */
#ifndef CGCOLLECTOR2_NUMOPERATIONSMD_H
#define CGCOLLECTOR2_NUMOPERATIONSMD_H

#include "metadata/MetaData.h"

class NumOperationsMD : public metacg::MetaData::Registrar<NumOperationsMD> {
 public:
  NumOperationsMD() = default;
  static constexpr const char* key = "numOperations";

  explicit NumOperationsMD(const nlohmann::json& j) {
    if (j.is_null()) {
      metacg::MCGLogger::instance().getConsole()->error("Could not retrieve meta data for {}", key);
      return;
    }
    int jNumberOfIntOps = j["numberOfIntOps"].get<int>();
    int jNumberOfFloatOps = j["numberOfFloatOps"].get<int>();
    int jNumberOfControlFlowOps = j["numberOfControlFlowOps"].get<int>();
    int jNumberOfMemoryAccesses = j["numberOfMemoryAccesses"].get<int>();
    numberOfIntOps = jNumberOfIntOps;
    numberOfFloatOps = jNumberOfFloatOps;
    numberOfControlFlowOps = jNumberOfControlFlowOps;
    numberOfMemoryAccesses = jNumberOfMemoryAccesses;
  }

 private:
  NumOperationsMD(const NumOperationsMD& other)
      : numberOfIntOps(other.numberOfIntOps),
        numberOfFloatOps(other.numberOfFloatOps),
        numberOfControlFlowOps(other.numberOfControlFlowOps),
        numberOfMemoryAccesses(other.numberOfMemoryAccesses) {}

 public:
  nlohmann::json to_json() const final {
    nlohmann::json j;
    j["numberOfIntOps"] = numberOfIntOps;
    j["numberOfFloatOps"] = numberOfFloatOps;
    j["numberOfControlFlowOps"] = numberOfControlFlowOps;
    j["numberOfMemoryAccesses"] = numberOfMemoryAccesses;
    return j;
  }

  virtual const char* getKey() const final { return key; }

  void merge(const MetaData& toMerge) final {
    assert(toMerge.getKey() == getKey() && "Trying to merge NumOperationsMD with meta data of different types");

    const NumOperationsMD* toMergeDerived = static_cast<const NumOperationsMD*>(&toMerge);

    if (toMergeDerived->numberOfIntOps != numberOfIntOps || toMergeDerived->numberOfFloatOps != numberOfFloatOps ||
        toMergeDerived->numberOfControlFlowOps != numberOfControlFlowOps ||
        toMergeDerived->numberOfMemoryAccesses != numberOfMemoryAccesses) {
      numberOfIntOps += toMergeDerived->numberOfIntOps;
      numberOfFloatOps += toMergeDerived->numberOfFloatOps;
      numberOfControlFlowOps += toMergeDerived->numberOfControlFlowOps;
      numberOfMemoryAccesses += toMergeDerived->numberOfMemoryAccesses;

      if ((numberOfIntOps != 0 && toMergeDerived->numberOfIntOps != 0 &&
           numberOfIntOps != toMergeDerived->numberOfIntOps) ||
          (numberOfFloatOps != 0 && toMergeDerived->numberOfFloatOps != 0 &&
           numberOfFloatOps != toMergeDerived->numberOfFloatOps) ||
          (numberOfControlFlowOps != 0 && toMergeDerived->numberOfControlFlowOps != 0 &&
           numberOfControlFlowOps != toMergeDerived->numberOfControlFlowOps) ||
          (numberOfMemoryAccesses != 0 && toMergeDerived->numberOfMemoryAccesses != 0 &&
           numberOfMemoryAccesses != toMergeDerived->numberOfMemoryAccesses)) {
        metacg::MCGLogger::instance().getErrConsole()->warn(
            "Same function defined with different number of operations found on merge.");
      }
    }
  }

  MetaData* clone() const final { return new NumOperationsMD(*this); }

  int numberOfIntOps{0};
  int numberOfFloatOps{0};
  int numberOfControlFlowOps{0};
  int numberOfMemoryAccesses{0};
};

#endif  // CGCOLLECTOR2_NUMOPERATIONSMD_H

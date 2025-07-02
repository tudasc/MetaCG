/**
* File: NumConditionalBranchMD.h
* License: Part of the MetaCG project. Licensed under BSD 3 clause license. See LICENSE.txt file at
* https://github.com/tudasc/metacg/LICENSE.txt
 */
#ifndef CGCOLLECTOR2_NUMCONDITIONALBRANCHMD_H
#define CGCOLLECTOR2_NUMCONDITIONALBRANCHMD_H

#include "metadata/MetaData.h"

class NumConditionalBranchMD : public metacg::MetaData::Registrar<NumConditionalBranchMD> {
 public:
  static constexpr const char* key = "numConditionalBranches";
  NumConditionalBranchMD() = default;
  explicit NumConditionalBranchMD(const nlohmann::json& j) {
    if (j.is_null()) {
      metacg::MCGLogger::instance().getConsole()->error("Could not retrieve meta data for {}", key);
      return;
    }
    int numberOfConditionalBranches = j.get<int>();
    numConditionalBranches = numberOfConditionalBranches;
  }

 private:
  NumConditionalBranchMD(const NumConditionalBranchMD& other)
      : numConditionalBranches(other.numConditionalBranches) {}

 public:
  nlohmann::json to_json() const final { return numConditionalBranches; }

  virtual const char* getKey() const final { return key; }

  void merge(const MetaData& toMerge) final {
    assert(toMerge.getKey() == getKey() && "Trying to merge NumConditionalBranchMD with meta data of different types");

    const NumConditionalBranchMD* toMergeDerived = static_cast<const NumConditionalBranchMD*>(&toMerge);
    
    if (numConditionalBranches != toMergeDerived->numConditionalBranches) {
      numConditionalBranches += toMergeDerived->numConditionalBranches;

      if (numConditionalBranches != 0 && toMergeDerived->numConditionalBranches != 0) {
        metacg::MCGLogger::instance().getErrConsole()->warn(
            "Same function defined with different number of conditional branches found on merge.");
      }
    }
  }

  std::unique_ptr<MetaData> clone() const final { return std::unique_ptr<MetaData>(new NumConditionalBranchMD(*this)); }

  int numConditionalBranches{0};
};

#endif  // CGCOLLECTOR2_NUMCONDITIONALBRANCHMD_H

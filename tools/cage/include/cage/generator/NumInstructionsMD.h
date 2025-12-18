/**
 * File: NumInstructionsMD.h
 * License: Part of the MetaCG project. Licensed under BSD 3 clause license. See LICENSE.txt file at
 * https://github.com/tudasc/metacg/LICENSE.txt
 */
#ifndef CAGE_NUMINSTRUCTIONSMD_H
#define CAGE_NUMINSTRUCTIONSMD_H

#include "metadata/MetaData.h"

using namespace metacg;

namespace cage {

class NumInstructionsMD : public metacg::MetaData::Registrar<NumInstructionsMD> {
 public:
  static constexpr const char* key = "numInstructions";
  NumInstructionsMD() = default;
  explicit NumInstructionsMD(const nlohmann::json& j, StrToNodeMapping&) {
    metacg::MCGLogger::instance().getConsole()->trace("Reading NumInstructionsMD from json");
    if (j.is_null()) {
      metacg::MCGLogger::instance().getConsole()->trace("Could not retrieve meta data for {}", "NumInstructionsMD");
      return;
    }
    auto jsonnumInstructions = j.get<long long int>();
    setNumberOfInstructions(jsonnumInstructions);
  }

  explicit NumInstructionsMD(int num) : numInstructions(num) {}

 private:
  NumInstructionsMD(const NumInstructionsMD& other) : numInstructions(other.numInstructions) {}

 public:
  nlohmann::json toJson(NodeToStrMapping&) const final { return getNumberOfInstructions(); }

  const char* getKey() const override { return key; }

  void merge(const MetaData& toMerge, std::optional<MergeAction>, const GraphMapping&) final {
    assert(toMerge.getKey() == getKey() && "Trying to merge NumInstructionsMD with meta data of different types");

    const NumInstructionsMD* toMergeDerived = static_cast<const NumInstructionsMD*>(&toMerge);

    if (numInstructions != 0 && toMergeDerived->getNumberOfInstructions() != 0 &&
        numInstructions != toMergeDerived->getNumberOfInstructions()) {
      metacg::MCGLogger::instance().getErrConsole()->warn(
          "Same function defined with different number of instructions found on merge.");
    }
    numInstructions = std::max(numInstructions, toMergeDerived->getNumberOfInstructions());
  }

  std::unique_ptr<MetaData> clone() const final { return std::unique_ptr<MetaData>(new NumInstructionsMD(*this)); }

  void applyMapping(const GraphMapping&) override {}

  void setNumberOfInstructions(int numInstructions) { this->numInstructions = numInstructions; }
  int getNumberOfInstructions() const { return this->numInstructions; }

 private:
  int numInstructions{0};
};
}  // namespace metacg

#endif  // CAGE_NUMINSTRUCTIONSMD_H

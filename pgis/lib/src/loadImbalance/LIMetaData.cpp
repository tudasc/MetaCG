/**
 * File: LIMetaData.cpp
 * License: Part of the metacg project. Licensed under BSD 3 clause license. See LICENSE.txt file at
 * https://github.com/tudasc/metacg/LICENSE.txt
 */

#include "loadImbalance/LIMetaData.h"

using namespace LoadImbalance;

std::basic_string<char> flagType2String(FlagType type) {
  if (type == FlagType::Irrelevant) {
    return "irrelevant";
  }
  if (type == FlagType::Visited) {
    return "visited";
  }
  return "";
}

void LIMetaData::flag(FlagType type) { this->flags[type] = true; }

bool LIMetaData::isFlagged(FlagType type) const { return this->flags.at(type); }

void LIMetaData::setAssessment(double assessment) { this->assessment = std::optional<double>(assessment); }

std::optional<double> LIMetaData::getAssessment() { return this->assessment; }
nlohmann::json LIMetaData::to_json() const {
  return nlohmann::json{
      {flagType2String(FlagType::Visited), isFlagged(FlagType::Visited)},
      {flagType2String(FlagType::Irrelevant), isFlagged(FlagType::Irrelevant)},
  };
}

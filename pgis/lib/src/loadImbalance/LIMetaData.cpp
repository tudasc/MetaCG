/**
 * File: LIMetaData.cpp
 * License: Part of the MetaCG project. Licensed under BSD 3 clause license. See LICENSE.txt file at https://github.com/tudasc/metacg/LICENSE.txt
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

void LIMetaData::flag(FlagType type) {
  this->flags[type] = true;
}

bool LIMetaData::isFlagged(FlagType type) const {
  return this->flags.at(type);
}

void LIMetaData::setAssessment(double assessment) {
  this->assessment = std::optional<double>(assessment);
}

std::optional<double> LIMetaData::getAssessment() {
  return this->assessment;
}

void LoadImbalance::to_json(nlohmann::json& j, const LoadImbalance::LIMetaData& d) {
  j = nlohmann::json {
      {flagType2String(FlagType::Visited), d.isFlagged(FlagType::Visited)},
      {flagType2String(FlagType::Irrelevant), d.isFlagged(FlagType::Irrelevant)},
  };
}
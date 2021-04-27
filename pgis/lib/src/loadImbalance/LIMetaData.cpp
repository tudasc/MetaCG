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
  switch (type) {
    case FlagType::Visited:
      this->visited = true;
      break;
    case FlagType::Irrelevant:
      this->irrelevant = true;
      break;
  }
}

bool LIMetaData::isFlagged(FlagType type) const {
  switch (type) {
    case FlagType::Visited:
      return this->visited;
    case FlagType::Irrelevant:
      return this->irrelevant;
  }
}

void LIMetaData::setAssessment(double assessment) {
  this->assessment = std::optional<double>(assessment);
}

std::optional<double> LIMetaData::getAssessment() {
  return this->assessment;
}

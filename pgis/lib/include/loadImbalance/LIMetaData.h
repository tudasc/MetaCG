/**
 * File: LIMetaData.h
 * License: Part of the MetaCG project. Licensed under BSD 3 clause license. See LICENSE.txt file at
 * https://github.com/tudasc/metacg/LICENSE.txt
 */

#ifndef LI_METADATA_H
#define LI_METADATA_H

#include <optional>

#include "CgNodeMetaData.h"
#include "nlohmann/json.hpp"

namespace LoadImbalance {

/**
 * Available flags which can be annotated to a node.
 */
enum class FlagType { Irrelevant, Visited };

/**
 * Class to hold data (for load imbalance detection) which can be annotated to a node
 */
class LIMetaData : public pira::MetaData {
 public:
  static constexpr const char *key() { return "LIMetaData"; }

  void setNumberOfInclusiveStatements(pira::Statements inclusiveStatements) {
    this->inclusiveStatements = inclusiveStatements;
  }
  pira::Statements getNumberOfInclusiveStatements() const { return this->inclusiveStatements; }

  bool isVirtual() const { return this->isVirtualFunction; }
  void setVirtual(bool isVirtual) { this->isVirtualFunction = isVirtual; }

  void flag(FlagType type);
  bool isFlagged(FlagType type) const;

  void setAssessment(double assessment);
  std::optional<double> getAssessment();

  NLOHMANN_DEFINE_TYPE_INTRUSIVE(LIMetaData, visited, irrelevant);

 private:
  pira::Statements inclusiveStatements{0};
  bool isVirtualFunction{false};

  // Flags
  // =====
  bool visited{false};
  bool irrelevant{false};

  /**
   * to save the calculated metric value for later use
   */
  std::optional<double> assessment{std::nullopt};
};

}  // namespace LoadImbalance

#endif

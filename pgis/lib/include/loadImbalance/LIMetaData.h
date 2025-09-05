/**
 * File: LIMetaData.h
 * License: Part of the metacg project. Licensed under BSD 3 clause license. See LICENSE.txt file at
 * https://github.com/tudasc/metacg/LICENSE.txt
 */

#ifndef LI_METADATA_H
#define LI_METADATA_H

#include <optional>

#include "MetaData/CgNodeMetaData.h"
#include "metadata/MetaData.h"
#include "nlohmann/json.hpp"

namespace LoadImbalance {

/**
 * Available flags which can be annotated to a node.
 * Irrelevant and Visited will be carried over to the next iteration. Imbalanced is to be recalculated every time.
 */
enum class FlagType { Irrelevant, Visited, Imbalanced };

/**
 * Class to hold data (for load imbalance detection) which can be annotated to a node
 */
class LIMetaData : public metacg::MetaData::Registrar<LIMetaData> {
 public:
  static constexpr const char* key = "LIData";
  LIMetaData() = default;
  explicit LIMetaData(const nlohmann::json& j, metacg::StrToNodeMapping&) {
    if (j.is_null()) {
      metacg::MCGLogger::instance().getConsole()->trace("Could not retrieve meta data for {}", key);
      return;
    }

    bool irrelevant = j["irrelevant"].get<bool>();
    bool visited = j["visited"].get<bool>();

    if (irrelevant) {
      metacg::MCGLogger::instance().getConsole()->debug("Setting flag irrelevant");
      flag(LoadImbalance::FlagType::Irrelevant);
    }
    if (visited) {
      metacg::MCGLogger::instance().getConsole()->debug("Setting flag visited");
      flag(LoadImbalance::FlagType::Visited);
    }
  };

 private:
  LIMetaData(const LIMetaData& other)
      : inclusiveStatements(other.inclusiveStatements),
        isVirtualFunction(other.isVirtualFunction),
        flags(other.flags),
        assessment(other.assessment) {}

 public:
  virtual nlohmann::json toJson(metacg::NodeToStrMapping&) const override;

  [[nodiscard]] const char* getKey() const final { return key; }

  void merge(const MetaData& toMerge, std::optional<metacg::MergeAction>, const metacg::GraphMapping&) final {
    if (std::strcmp(toMerge.getKey(), getKey()) != 0) {
      metacg::MCGLogger::instance().getErrConsole()->error(
          "The MetaData which was tried to merge with LIMetaData was of a different MetaData type");
      abort();
    }

    // const LIMetaData* toMergeDerived = static_cast<const LIMetaData*>(&toMerge);

    // TODO: Merge semantics?
  }

  void applyMapping(const metacg::GraphMapping&) override {}

  std::unique_ptr<MetaData> clone() const final { return std::unique_ptr<MetaData>(new LIMetaData(*this)); }

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

 private:
  pira::Statements inclusiveStatements{0};
  bool isVirtualFunction{false};

  // Flags
  // =====
  std::unordered_map<FlagType, bool> flags{
      {FlagType::Visited, false}, {FlagType::Irrelevant, false}, {FlagType::Imbalanced, false}};

  /**
   * to save the calculated metric value for later use
   */
  std::optional<double> assessment{std::nullopt};
};

void to_json(nlohmann::json& j, const LoadImbalance::LIMetaData& d);
}  // namespace LoadImbalance

#endif

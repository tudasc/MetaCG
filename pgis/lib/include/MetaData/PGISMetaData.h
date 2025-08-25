/**
 * File: PGISMetaData.h
 * License: Part of the MetaCG project. Licensed under BSD 3 clause license. See LICENSE.txt file at
 * https://github.com/tudasc/metacg/LICENSE.txt
 */

#ifndef METACG_PGISMETADATA_H
#define METACG_PGISMETADATA_H

#include "CgNode.h"
#include "LoggerUtil.h"
#include "metadata/MetaData.h"

namespace metacg::pgis {

/**
 * MetaData to encode if a node is marked for instrumentation (Instrumented, function-level instrumentation) or for path
 * instrumentation (InstrumentedPath, call-site instrumentation).
 */
class InstrumentationMetaData : public metacg::MetaData::Registrar<InstrumentationMetaData> {
 public:
  static constexpr const char* key = "InstrumentationMetaData";
  InstrumentationMetaData() : state(InstrumentationState::None) {}
  InstrumentationMetaData(const nlohmann::json& j, StrToNodeMapping&) : state(InstrumentationState::None) {
    metacg::MCGLogger::instance().getConsole()->warn("This constructor is not supposed to be called");
  }

 private:
  InstrumentationMetaData(const InstrumentationMetaData& other) : state(other.state) {}

 public:
  nlohmann::json toJson(NodeToStrMapping&) const final {
    metacg::MCGLogger::instance().getConsole()->trace("Serializing InstrumentationMetaData to json is not implemented");
    return {};
  };

  const char* getKey() const final { return key; }

  void merge(const MetaData& toMerge, std::optional<MergeAction>, const GraphMapping&) final {
    if (std::strcmp(toMerge.getKey(), getKey()) != 0) {
      metacg::MCGLogger::instance().getErrConsole()->error(
          "The MetaData which was tried to merge with InstrumentationMetaData was of a different MetaData type");
      abort();
    }

    metacg::MCGLogger::instance().getErrConsole()->warn(
        "InstrumentationMetaData should be written into a fully merged graph directly.");
  }

  void applyMapping(const GraphMapping&) override {}

  std::unique_ptr<MetaData> clone() const final {
    return std::unique_ptr<MetaData>(new InstrumentationMetaData(*this));
  }

  void reset() { state = InstrumentationState::None; }
  void setInstrumented() { state = InstrumentationState::Instrumented; }
  void setInstrumentedPath() { state = InstrumentationState::InstrumentedPath; }
  [[nodiscard]] bool isInstrumented() const { return state == InstrumentationState::Instrumented; }
  [[nodiscard]] bool isInstrumentedPath() const { return state == InstrumentationState::InstrumentedPath; }

 private:
  enum class InstrumentationState { None, Instrumented, InstrumentedPath };

  InstrumentationState state;
};

inline void resetInstrumentation(metacg::CgNode* node) {
  auto md = &node->getOrCreate<InstrumentationMetaData>();
  md->reset();
}

inline void instrumentNode(metacg::CgNode* node) {
  auto md = &node->getOrCreate<InstrumentationMetaData>();
  md->setInstrumented();
}

inline void instrumentPathNode(metacg::CgNode* node) {
  auto md = &node->getOrCreate<InstrumentationMetaData>();
  md->setInstrumentedPath();
}

[[nodiscard]] inline bool isInstrumented(metacg::CgNode* node) {
  auto md = &node->getOrCreate<InstrumentationMetaData>();
  return md->isInstrumented();
}

[[nodiscard]] inline bool isInstrumentedPath(metacg::CgNode* node) {
  auto md = &node->getOrCreate<InstrumentationMetaData>();
  return md->isInstrumentedPath();
}

[[nodiscard]] inline bool isAnyInstrumented(metacg::CgNode* node) {
  auto md = &node->getOrCreate<InstrumentationMetaData>();
  return md->isInstrumented() || md->isInstrumentedPath();
}

}  // namespace metacg::pgis
#endif  // METACG_PGISMETADATA_H

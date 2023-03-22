/**
 * File: PGISMetaData.h
 * License: Part of the MetaCG project. Licensed under BSD 3 clause license. See LICENSE.txt file at
 * https://github.com/tudasc/metacg/LICENSE.txt
 */

#ifndef METACG_PGISMETADATA_H
#define METACG_PGISMETADATA_H

#include "CgNode.h"
#include "LoggerUtil.h"
#include "MetaData.h"

namespace pgis {

/**
 * MetaData to encode if a node is marked for instrumentation (Instrumented, function-level instrumentation) or for path
 * instrumentation (InstrumentedPath, call-site instrumentation).
 */
class InstrumentationMetaData : public metacg::MetaData::Registrar<InstrumentationMetaData> {
 public:
  static constexpr const char *key = "InstrumentationMetaData";
  InstrumentationMetaData() : state(InstrumentationState::None) {}
  InstrumentationMetaData(const nlohmann::json &j) : state(InstrumentationState::None) {
    metacg::MCGLogger::instance().getConsole()->warn("This constructor is not supposed to be called");
  }
  nlohmann::json to_json() const final {
    metacg::MCGLogger::instance().getConsole()->trace("Serializing InstrumentationMetaData to json is not implemented");
    return {};
  };

  void reset() { state = InstrumentationState::None; }
  void setInstrumented() { state = InstrumentationState::Instrumented; }
  void setInstrumentedPath() { state = InstrumentationState::InstrumentedPath; }
  [[nodiscard]] bool isInstrumented() const { return state == InstrumentationState::Instrumented; }
  [[nodiscard]] bool isInstrumentedPath() const { return state == InstrumentationState::InstrumentedPath; }

 private:
  enum class InstrumentationState { None, Instrumented, InstrumentedPath };

  InstrumentationState state;
};

inline void resetInstrumentation(metacg::CgNode *node) {
  auto md = node->getOrCreateMD<InstrumentationMetaData>();
  md->reset();
}

inline void instrumentNode(metacg::CgNode *node) {
  auto md = node->getOrCreateMD<InstrumentationMetaData>();
  md->setInstrumented();
}

inline void instrumentPathNode(metacg::CgNode *node) {
  auto md = node->getOrCreateMD<InstrumentationMetaData>();
  md->setInstrumentedPath();
}

inline bool isInstrumented(metacg::CgNode *node) {
  auto md = node->getOrCreateMD<InstrumentationMetaData>();
  return md->isInstrumented();
}

inline bool isInstrumentedPath(metacg::CgNode *node) {
  auto md = node->getOrCreateMD<InstrumentationMetaData>();
  return md->isInstrumentedPath();
}

}  // namespace pgis
#endif  // METACG_PGISMETADATA_H

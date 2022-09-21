/**
 * File: PGISMetaData.h
 * License: Part of the MetaCG project. Licensed under BSD 3 clause license. See LICENSE.txt file at
 * https://github.com/tudasc/metacg/LICENSE.txt
 */

#ifndef METACG_PGISMETADATA_H
#define METACG_PGISMETADATA_H

#include "CgNode.h"
#include "MetaData.h"

namespace pgis {

/**
 * MetaData to encode if a node is marked for instrumentation (Instrumented, function-level instrumentation) or for path
 * instrumentation (InstrumentedPath, call-site instrumentation).
 */
class InstrumentationMetaData : public metacg::MetaData {
 public:
  static constexpr const char *key() { return "InstrumentationMetaData"; }
  InstrumentationMetaData() : state(InstrumentationState::None) {}

  void reset() { state = InstrumentationState::None; }
  void setInstrumented() { state = InstrumentationState::Instrumented; }
  void setInstrumentedPath() { state = InstrumentationState::InstrumentedPath; }
  [[nodiscard]] bool isInstrumented() const { return state == InstrumentationState::Instrumented; }
  [[nodiscard]] bool isInstrumentedPath() const { return state == InstrumentationState::InstrumentedPath; }

 private:
  enum class InstrumentationState { None, Instrumented, InstrumentedPath };

  InstrumentationState state;
};

inline void resetInstrumentation(CgNodePtr node) {
  auto md = getOrCreateMD<InstrumentationMetaData>(std::move(node));
  md->reset();
}

inline void instrumentNode(CgNodePtr node) {
  auto md = getOrCreateMD<InstrumentationMetaData>(std::move(node));
  md->setInstrumented();
}

inline void instrumentPathNode(CgNodePtr node) {
  auto md = getOrCreateMD<InstrumentationMetaData>(std::move(node));
  md->setInstrumentedPath();
}

inline bool isInstrumented(CgNodePtr node) {
  auto md = getOrCreateMD<InstrumentationMetaData>(std::move(node));
  return md->isInstrumented();
}

inline bool isInstrumentedPath(CgNodePtr node) {
  auto md = getOrCreateMD<InstrumentationMetaData>(std::move(node));
  return md->isInstrumentedPath();
}

}  // namespace pgis
#endif  // METACG_PGISMETADATA_H

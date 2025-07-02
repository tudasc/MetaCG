/**
 * File: InlineMD.h
 * License: Part of the MetaCG project. Licensed under BSD 3 clause license. See LICENSE.txt file at
 * https://github.com/tudasc/metacg/LICENSE.txt
 */
#ifndef METACG_INLINEMD_H
#define METACG_INLINEMD_H

#include "metadata/MetaData.h"

class InlineMD : public metacg::MetaData::Registrar<InlineMD> {
 public:
  static constexpr const char* key = "inlineInfo";
  InlineMD() = default;
  explicit InlineMD(const nlohmann::json& j) {
    metacg::MCGLogger::instance().getConsole()->trace("Reading inlineInfo from json");
    if (j.is_null()) {
      metacg::MCGLogger::instance().getConsole()->trace("Could not retrieve meta data for {}", "inlineInfo");
      return;
    }
    markedInline = j["markedInline"];
    likelyInline = j["likelyInline"];
    markedAlwaysInline = j["markedAlwaysInline"];
    isTemplateFunction = j["isTemplate"];
  }

 private:
  InlineMD(const InlineMD& other) = default;

 public:
  nlohmann::json to_json() const final {
    nlohmann::json j;
    j["markedInline"] = markedInline;
    j["likelyInline"] = likelyInline;
    j["isTemplate"] = isTemplateFunction;
    j["markedAlwaysInline"] = markedAlwaysInline;
    return j;
  };

  const char* getKey() const override { return key; }

  void merge(const MetaData& toMerge) final {
    assert(toMerge.getKey() == getKey() && "Trying to merge InlineMD with metadata of different types");

    const InlineMD* toMergeDerived = static_cast<const InlineMD*>(&toMerge);
    if (this->isTemplateFunction != toMergeDerived->isTemplateFunction) {
      metacg::MCGLogger::instance().getErrConsole()->warn("Merging functions with mismatched 'isTemplate' metadata");
    }
    // For the rest of the metrics, adopt the stronger attributes
    markedInline |= toMergeDerived->markedInline;
    likelyInline |= toMergeDerived->likelyInline;
    markedAlwaysInline |= toMergeDerived->markedAlwaysInline;
  }

  std::unique_ptr<MetaData> clone() const final { return std::unique_ptr<MetaData>(new InlineMD(*this)); }

  bool isMarkedInline() const { return markedInline; }

  bool isLikelyInline() const { return likelyInline; }

  bool isMarkedAlwaysInline() const { return markedAlwaysInline; }

  bool isTemplate() const { return isTemplateFunction; }

 private:
  bool markedInline = false;
  bool likelyInline = false;
  bool markedAlwaysInline = false;
  bool isTemplateFunction = false;
};

#endif  // METACG_INLINEMD_H

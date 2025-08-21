/**
 * File: DummyMD.cpp
 * License: Part of the metacg project. Licensed under BSD 3 clause license. See LICENSE.txt file at
 * https://github.com/tudasc/metacg/LICENSE.txt
 */

#include <memory>
#include <metadata/MetaData.h>

class DummyMD : public metacg::MetaData::Registrar<DummyMD> {
 public:
  static constexpr const char* key = "dummy_md";
  DummyMD() = default;
  explicit DummyMD(const nlohmann::json& j, metacg::StrToNodeMapping&) {}

  virtual void merge(const MetaData& toMerge) final {}
  void merge(const MetaData& toMerge, std::optional<metacg::MergeAction>, const metacg::GraphMapping&) final {}

  virtual std::unique_ptr<MetaData> clone() const final { return std::make_unique<DummyMD>(*this); }

  virtual nlohmann::json toJson(metacg::NodeToStrMapping& nodeToStr) const final {
    return {{"key1", "some string"}, {"key2", 42}};
  };

  virtual const char* getKey() const override { return key; }

  void applyMapping(const metacg::GraphMapping&) override {}
};
#include <metadata/MetaData.h>

class DummyMD : public metacg::MetaData::Registrar<DummyMD> {
 public:
  static constexpr const char* key = "dummy_md";
  DummyMD() = default;
  explicit DummyMD(const nlohmann::json& j) {}

  void merge(const MetaData& toMerge) final {}

  MetaData* clone() const final { return new DummyMD(*this); }

  nlohmann::json to_json() const final { return {{"key1", "some string"}, {"key2", 42}}; };

  virtual const char* getKey() const { return key; }
};
#include "metacg/io/IdMapping.h"
#include "metacg/metadata/MetaData.h"
#include <iostream>

namespace metacg {

enum class CallType { DIRECT, INDIRECT, VIRTUAL };
class CallTypeMD final : public metacg::MetaData::Registrar<CallTypeMD> {
 public:
  CallType callType;

  static constexpr const char* key = "callType";
  CallTypeMD() = default;
  CallTypeMD(CallType callType) : callType(callType) {};
  CallTypeMD(const CallTypeMD& other) : callType(other.callType) {};

  explicit CallTypeMD(const nlohmann::json& j, StrToNodeMapping&) {
    metacg::MCGLogger::instance().getConsole()->trace("Creating {} metadata from json", key);
    if (j.is_null()) {
      metacg::MCGLogger::instance().getConsole()->trace("Could not retrieve metadata for {}", key);
      return;
    }

    if (!j.is_object() || !j.contains("type")) {
      throw std::runtime_error("CallTypeMD: Expected JSON object with 'type' field");
    }

    const std::string value = j["type"].get<std::string>();

    if (value == "DIRECT") {
      callType = CallType::DIRECT;
    } else if (value == "INDIRECT") {
      callType = CallType::INDIRECT;
    } else if (value == "VIRTUAL") {
      callType = CallType::VIRTUAL;
    } else {
      throw std::runtime_error("Unknown CallType: " + value);
    }
  }

  nlohmann::json toJson(NodeToStrMapping&) const override {
    switch (callType) {
      case CallType::DIRECT:
        return {{"type", "DIRECT"}};
      case CallType::INDIRECT:
        return {{"type", "INDIRECT"}};
      case CallType::VIRTUAL:
        return {{"type", "VIRTUAL"}};
    }

    return nullptr;
  }

  const char* getKey() const override { return key; }

  std::unique_ptr<MetaData> clone() const final { return std::unique_ptr<MetaData>(new CallTypeMD(*this)); }

  void merge(const MetaData& toMerge, std::optional<metacg::MergeAction>, const metacg::GraphMapping&) final override {
    assert(toMerge.getKey() == getKey() && "Trying to merge CallTypeMD with meta data of different types");

    const CallTypeMD* toMergeDerived = static_cast<const CallTypeMD*>(&toMerge);

    if (toMergeDerived->callType != callType) {
      metacg::MCGLogger::instance().getErrConsole()->warn("Same edge defined with different callTypes.");
    }
  }

  void applyMapping(const GraphMapping&) override {}
};
}  // namespace metacg

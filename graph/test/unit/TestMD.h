/**
 * File: TestMD.h
 * License: Part of the metacg project. Licensed under BSD 3 clause license. See LICENSE.txt file at
 * https://github.com/tudasc/metacg/LICENSE.txt
 */
#ifndef METACG_TESTMD_H
#define METACG_TESTMD_H

#include "metacg/io/VersionFourMCGReader.h"
#include "metacg/metadata/MetaData.h"

struct SimpleTestMD final : metacg::MetaData::Registrar<SimpleTestMD> {
  static constexpr const char* key = "SimpleTestMD";

  explicit SimpleTestMD(const nlohmann::json& j, metacg::StrToNodeMapping&) {
    stored_int = j.at("stored_int");
    stored_double = j.at("stored_double");
    stored_string = j.at("stored_string");
  }

  explicit SimpleTestMD(int storeInt, double storeDouble, const std::string& storeString) {
    stored_int = storeInt;
    stored_double = storeDouble;
    stored_string = storeString;
  }

 private:
  SimpleTestMD(const SimpleTestMD& other)
      : stored_int(other.stored_int), stored_double(other.stored_double), stored_string(other.stored_string) {}

 public:
  nlohmann::json toJson(metacg::NodeToStrMapping&) const final {
    return {{"stored_int", stored_int}, {"stored_double", stored_double}, {"stored_string", stored_string}};
  };

  const char* getKey() const override { return key; }

  void merge(const MetaData& toMerge, std::optional<metacg::MergeAction> action, const metacg::GraphMapping&) final {
    if (std::strcmp(toMerge.getKey(), getKey()) != 0) {
      metacg::MCGLogger::logError(
          "The MetaData which was tried to merge with SimpleTestMD was of a different MetaData type");
      abort();
    }
  }

  std::unique_ptr<MetaData> clone() const final { return std::unique_ptr<SimpleTestMD>(new SimpleTestMD(*this)); }

  void applyMapping(const metacg::GraphMapping&) override {}

 public:
  int stored_int;
  double stored_double;
  std::string stored_string;
};

struct RefTestMD final : metacg::MetaData::Registrar<RefTestMD> {
  static constexpr const char* key = "RefTestMD";

  explicit RefTestMD(const nlohmann::json& j, metacg::StrToNodeMapping& strToNode) {
    auto nodeRefStr = j.at("node_ref");
    auto* node = strToNode.getNodeFromStr(nodeRefStr);
    this->nodeRef = node->getId();
  }

  explicit RefTestMD(metacg::CgNode& node) : nodeRef(node.getId()) {}

 private:
  RefTestMD(const RefTestMD& other) = default;

 public:
  nlohmann::json toJson(metacg::NodeToStrMapping& nodeToStr) const final {
    auto nodeRefStr = nodeToStr.getStrFromNode(nodeRef);
    return {{"node_ref", nodeRefStr}};
  };

  const char* getKey() const override { return key; }

  void merge(const MetaData& toMerge, std::optional<metacg::MergeAction> action,
             const metacg::GraphMapping& mapping) final {
    if (std::strcmp(toMerge.getKey(), getKey()) != 0) {
      metacg::MCGLogger::logError(
          "The MetaData which was tried to merge with RefTestMD was of a different MetaData type");
      abort();
    }
    auto& toMergeDerived = static_cast<const RefTestMD&>(toMerge);

    if (action) {
      // There is a merge action -> this metadata is attached to a node.
      // Behavior here is to overwrite the data if the node is replaced.
      if (action->replace) {
        this->nodeRef = mapping.at(toMergeDerived.nodeRef);
      }
    } else {
      // There is no merge action -> this metadata is attached globally.
      // Behavior: simply keep the existing node ref.
    }
  }

  std::unique_ptr<MetaData> clone() const final { return std::unique_ptr<RefTestMD>(new RefTestMD(*this)); }

  void applyMapping(const metacg::GraphMapping& mapping) override { nodeRef = mapping.at(nodeRef); }

  metacg::NodeId getNodeRef() const { return nodeRef; }

 private:
  metacg::NodeId nodeRef;
};

/**
 * Metadata that is never written to the output.
 * intentional == true:  toJson returns an empty object ({}), i.e. the metadata intentionally does not want to be
 *                serialized. The writer is expected to silently skip it.
 * intentional == false: toJson returns null, i.e. serialization failed. The writer is expected to skip it and warn.
 */
template <bool intentional>
struct ExportEmptyTestMD final : metacg::MetaData::Registrar<ExportEmptyTestMD<intentional>> {
  static constexpr const char* key = intentional ? "EmptyExportTestMD" : "NullExportTestMD";

  explicit ExportEmptyTestMD(const nlohmann::json&, metacg::StrToNodeMapping&) {}
  ExportEmptyTestMD() = default;

 private:
  ExportEmptyTestMD(const ExportEmptyTestMD&) = default;

 public:
  nlohmann::json toJson(metacg::NodeToStrMapping&) const final {
    return intentional ? nlohmann::json::object() : nlohmann::json(nullptr);
  };

  const char* getKey() const override { return key; }

  void merge(const metacg::MetaData&, std::optional<metacg::MergeAction>, const metacg::GraphMapping&) final {}

  std::unique_ptr<metacg::MetaData> clone() const final {
    return std::unique_ptr<ExportEmptyTestMD>(new ExportEmptyTestMD(*this));
  }

  void applyMapping(const metacg::GraphMapping&) override {}
};

#endif  // METACG_TESTMD_H

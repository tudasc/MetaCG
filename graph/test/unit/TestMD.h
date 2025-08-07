/**
 * File: TestMD.h
 * License: Part of the metacg project. Licensed under BSD 3 clause license. See LICENSE.txt file at
 * https://github.com/tudasc/metacg/LICENSE.txt
 */
#ifndef METACG_TESTMD_H
#define METACG_TESTMD_H

#include "io/VersionFourMCGReader.h"
#include "metadata/MetaData.h"

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
      metacg::MCGLogger::instance().getErrConsole()->error(
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
    this->nodeRef = node->id;
  }

  explicit RefTestMD(metacg::CgNode& node) { this->nodeRef = node.id; }

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
      metacg::MCGLogger::instance().getErrConsole()->error(
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

#endif  // METACG_TESTMD_H
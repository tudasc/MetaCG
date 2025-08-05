/**
* File: EntryFunctionMD.h
* License: Part of the MetaCG project. Licensed under BSD 3 clause license. See LICENSE.txt file at
* https://github.com/tudasc/metacg/LICENSE.txt
*/
#ifndef METACG_ENTRYFUNCTIONMD_H
#define METACG_ENTRYFUNCTIONMD_H

#include "metadata/MetaData.h"
#include "CgNode.h"

namespace metacg {

struct EntryFunctionMD final : metacg::MetaData::Registrar<EntryFunctionMD> {
 public:
  static constexpr const char* key = "entryFunction";
  EntryFunctionMD() = default;

  explicit EntryFunctionMD(const nlohmann::json& j, StrToNodeMapping& strToNode) {
    auto nodeStr = j.get<std::string>();
    auto node = strToNode.getNodeFromStr(nodeStr);
    if (!node) {
      MCGLogger::logError("Could not find entry function with ID '{}'", nodeStr);
      return;
    }
    entryFunction = node->getId();
  }

  explicit EntryFunctionMD(const CgNode& mainNode) {
    this->entryFunction = mainNode.getId();
  }

  nlohmann::json toJson(NodeToStrMapping& nodeToStr) const final {
    std::string nodeStr = "";
    if (entryFunction) {
      nodeStr = nodeToStr.getStrFromNode(*entryFunction);
    }
    return {nodeStr};
  }

 private:
  EntryFunctionMD(const EntryFunctionMD& other) : entryFunction(other.entryFunction) {}

 public:
  void merge(const MetaData& toMerge, std::optional<MergeAction> action, const GraphMapping& mapping) final {
    assert(toMerge.getKey() == getKey() && "Trying to merge EntryFunctionMD with meta data of different types");

    const EntryFunctionMD* toMergeDerived = static_cast<const EntryFunctionMD*>(&toMerge);

    if (action) {
      MCGLogger::logWarn("EntryFunctionMD is not meant to be attached to a node. Merging is not implemented for this "
          "case");
      return;
    }

    if (toMergeDerived->entryFunction) {
      if (!this->entryFunction) {
        entryFunction = mapping.at(*toMergeDerived->entryFunction);
      } else {
        MCGLogger::logWarn("The merged graphs both define an entry function. Keeping existing value (id={})",
                           *entryFunction);
      }
    }

  }

  std::unique_ptr<MetaData> clone() const final { return std::unique_ptr<MetaData>(new EntryFunctionMD(*this)); }

  void applyMapping(const GraphMapping& mapping) override {
    if (entryFunction) {
      entryFunction = mapping.at(*entryFunction);
    }
  }

  const char* getKey() const final { return key; }

  std::optional<NodeId> getEntryFunctionId() const {
    return entryFunction;
  }

 private:
  std::optional<NodeId> entryFunction;
};

} // metacg

#endif  // METACG_ENTRYFUNCTIONMD_H

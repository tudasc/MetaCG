/**
 * File: FunctionSignatureMetadata.h
 * License: Part of the MetaCG project. Licensed under BSD 3 clause license. See LICENSE.txt file at
 * https://github.com/tudasc/metacg/LICENSE.txt
 */

#ifndef CGCOLLECTOR2_FUNCTIONSIGNATUREMETADATA_H
#define CGCOLLECTOR2_FUNCTIONSIGNATUREMETADATA_H

#include "cgcollector2/SharedDefs.h"
#include "metacg/metadata/MetaData.h"

class FunctionSignatureMetadata : public metacg::MetaData::Registrar<FunctionSignatureMetadata> {
 public:
  static constexpr const char* key = "FunctionSignatureMetadata";

  FunctionSignatureMetadata() = default;
  FunctionSignatureMetadata(bool shouldExport) : shouldExport(shouldExport){};

  explicit FunctionSignatureMetadata(const nlohmann::json& j, metacg::StrToNodeMapping&) {
    metacg::MCGLogger::logTrace("FunctionSignatureMetadata from json");
    if (j.is_null()) {
      metacg::MCGLogger::logErrorUnique("Could not retrieve meta data for {}", key);
      return;
    }
    ownSignature = j;
  }

  FunctionSignatureMetadata(const FunctionSignatureMetadata& other)
      : ownSignature(other.ownSignature), shouldExport(other.shouldExport) {}

  const char* getKey() const final { return key; }

  nlohmann::json toJson(metacg::NodeToStrMapping&) const final {
    if (shouldExport) {
      return ownSignature;
    }
    return {};
  }

  void merge(const MetaData& toMerge, std::optional<metacg::MergeAction>, const metacg::GraphMapping&) final {
    if (std::strcmp(toMerge.getKey(), getKey()) != 0) {
      metacg::MCGLogger::logError(
          "The MetaData which was tried to merge with FunctionSignatureMetadata was of a different MetaData type");
      abort();
    }
    const auto* toMergeDerived = static_cast<const FunctionSignatureMetadata*>(&toMerge);
    // Signatures should usually be identical.
    // Some templates can have the same name and origin but differing function signatures.
    // If this is the case, we inform the user to use a stronger merge policy.
    if (!(ownSignature == toMergeDerived->ownSignature)) {
      metacg::MCGLogger::logDebug("Signature mismatch: {} <-> {}", ownSignature.toString(), toMergeDerived->ownSignature.toString());
      metacg::MCGLogger::logInfoUnique(
          "Found a signature mismatch between identically named nodes, this can be resolved by using a stronger merge "
          "policy");
    }
    // Either resolved by the merge policy (if the signature is taken into account),
    // or just keep any signature, as the graph is wrong anyway.
  }

  std::unique_ptr<MetaData> clone() const final { return std::make_unique<FunctionSignatureMetadata>(*this); }
  virtual void applyMapping(const metacg::GraphMapping&){};

  FunctionSignature ownSignature;
  bool shouldExport = true;
};
#endif  // CGCOLLECTOR2_FUNCTIONSIGNATUREMETADATA_H

/**
 * File: FunctionSignatureMetadata.h
 * License: Part of the MetaCG project. Licensed under BSD 3 clause license. See LICENSE.txt file at
 * https://github.com/tudasc/metacg/LICENSE.txt
 */

#ifndef CGCOLLECTOR2_FUNCTIONSIGNATUREMETADATA_H
#define CGCOLLECTOR2_FUNCTIONSIGNATUREMETADATA_H

#include <clang/AST/Type.h>

#include "SharedDefs.h"
#include "metadata/MetaData.h"

class FunctionSignatureMetadata : public metacg::MetaData::Registrar<FunctionSignatureMetadata> {
 public:
  static constexpr const char* key = "FunctionSignatureMetadata";

  FunctionSignatureMetadata() = default;
  FunctionSignatureMetadata(bool shouldExport) : shouldExport(shouldExport){};

  explicit FunctionSignatureMetadata(const nlohmann::json& j) {
    metacg::MCGLogger::instance().getConsole()->trace("FunctionSignatureMetadata from json");
    if (j.is_null()) {
      metacg::MCGLogger::instance().getConsole()->trace("Could not retrieve meta data for {}",
                                                        "FunctionSignatureMetadata");
      return;
    }
    ownSignature = j;
  }

 private:
  FunctionSignatureMetadata(const FunctionSignatureMetadata& other)
      : ownSignature(other.ownSignature), shouldExport(other.shouldExport) {}

 public:
  virtual const char* getKey() const { return key; }

  nlohmann::json to_json() const final {
    if (shouldExport) {
      return ownSignature;
    }
    return {};
  };

  // TODO implement
  void merge(const MetaData& toMerge) final {
    if (std::strcmp(toMerge.getKey(), getKey()) != 0) {
      metacg::MCGLogger::instance().getErrConsole()->error(
          "The MetaData which was tried to merge with FunctionSignatureMetadata was of a different MetaData type");
      abort();
    }
    const auto* toMergeDerived = static_cast<const FunctionSignatureMetadata*>(&toMerge);
    assert(ownSignature==toMergeDerived->ownSignature && "The two metadata should be equal");
  }

  MetaData* clone() const final { return new FunctionSignatureMetadata(*this); }

  FunctionSignature ownSignature;
  bool shouldExport = true;
};
#endif  // CGCOLLECTOR2_FUNCTIONSIGNATUREMETADATA_H

/**
* File: ASTNodeMetadata.h
* License: Part of the MetaCG project. Licensed under BSD 3 clause license. See LICENSE.txt file at
* https://github.com/tudasc/metacg/LICENSE.txt
*/

#ifndef CGCOLLECTOR2_ASTNODEMETADATA_H
#define CGCOLLECTOR2_ASTNODEMETADATA_H

#include "metadata/MetaData.h"
#include <clang/AST/Type.h>

class ASTNodeMetadata : public metacg::MetaData::Registrar<ASTNodeMetadata> {
 public:
  static constexpr const char* key = "ASTNodeMetadata";

  ASTNodeMetadata() = default;

  explicit ASTNodeMetadata(const nlohmann::json& j) {
    assert(false && "This metadata should not be created via json");
  }

  nlohmann::json to_json() const final {
    // This metadata will not be serialized as it contains memory locations that are run specific
    return {};
  };

  ASTNodeMetadata(clang::FunctionDecl* FD) : functionDecl(FD) {};

  const clang::FunctionDecl* getFunctionDecl() const { return functionDecl; }

  const char* getKey() const override { return key; }

  void merge(const MetaData& toMerge) final {
    if (std::strcmp(toMerge.getKey(), getKey()) != 0) {
      metacg::MCGLogger::instance().getErrConsole()->error(
          "The MetaData which was tried to merge with ASTNodeMetadata was of a different MetaData type");
      abort();
    }
    assert(false && "This metadata can not be exported and therefore is not mergeable");
    // const ASTNodeMetadata* toMergeDerived = static_cast<const ASTNodeMetadata*>(&toMerge);
  }

  MetaData* clone() const final {
    assert(false && "This should not be necesssary to use");
    return new ASTNodeMetadata();
  }

  void setFunctionDecl(const clang::FunctionDecl* const fD) { functionDecl = fD; }

 private:
  const clang::FunctionDecl* functionDecl;
};

#endif  // CGCOLLECTOR2_ASTNODEMETADATA_H

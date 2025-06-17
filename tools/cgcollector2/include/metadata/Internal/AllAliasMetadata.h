/**
* File: AllAliasMetadata.h
* License: Part of the MetaCG project. Licensed under BSD 3 clause license. See LICENSE.txt file at
* https://github.com/tudasc/metacg/LICENSE.txt
*/

#ifndef CGCOLLECTOR2_ALLALIASMETADATA_H
#define CGCOLLECTOR2_ALLALIASMETADATA_H

#include "metadata/MetaData.h"
#include <SharedDefs.h>
#include <clang/AST/Type.h>

/**
 * This class models the data necessary for the "AllAlias" assumption
 * The assumption is, that all functions matching a signature are possible call targets
 */
class AllAliasMetadata : public metacg::MetaData::Registrar<AllAliasMetadata> {
 public:
  static constexpr const char* key = "AllAliasMetadata";

  AllAliasMetadata() = default;
  AllAliasMetadata(bool shouldExport) : shouldExport(shouldExport) {}

  explicit AllAliasMetadata(const nlohmann::json& j) {
    metacg::MCGLogger::instance().getConsole()->trace("AllAliasMetadata from json");
    if (j.is_null()) {
      metacg::MCGLogger::instance().getConsole()->trace("Could not retrieve meta data for {}", "AllAliasMetadata");
      return;
    }
    mightCall = j;
  }

  virtual const char* getKey() const { return key; }

 private:
  AllAliasMetadata(const AllAliasMetadata& other) : mightCall(other.mightCall), shouldExport(other.shouldExport) {}

 public:
  nlohmann::json to_json() const final {
    if (shouldExport)
      return mightCall;
    else
      return {};
  };

  void merge(const MetaData& toMerge) final {
    if (std::strcmp(toMerge.getKey(), getKey()) != 0) {
      metacg::MCGLogger::instance().getErrConsole()->error(
          "The MetaData which was tried to merge with AllAliasMetadata was of a different MetaData type");
      abort();
    }

    if (std::strcmp(toMerge.getKey(), getKey()) != 0) {
      metacg::MCGLogger::instance().getErrConsole()->error(
          "The MetaData which was tried to merge with FunctionSignatureMetadata was of a different MetaData type");
      abort();
    }
    const auto* toMergeDerived = static_cast<const AllAliasMetadata*>(&toMerge);

    //This might be faster when using
    // mightCall.insert(mightCall.end(), toMergeDerived.begin(), toMergeDerived.end());
    // sort( mightCall.begin(), mightCall.end() );
    // mightCall.erase( unique( mightCall.begin(), mightCall.end() ), mightCall.end() );
    // but we would need a strict ordering relation between FunctionSignatureMetadata which we dont have
    mightCall.reserve(mightCall.size()+toMergeDerived->mightCall.size());
    for(const auto& elem : toMergeDerived->mightCall){
      if(std::find(mightCall.begin(), mightCall.end(), elem)!=mightCall.end()){
        mightCall.push_back(elem);
      }
    }

    //At this point the metadata should be merged so that the AllAliasMetadata contains all
    // signatures that are possibly used by the node, this metadata is attached to
    //FIXME: converting this to actual edges is not yet possible... I think?
    // We could hack this togeth, by adding another datamember containing the ids of the functions of interst
    // I decided to wait for more advanced metadata merge strategies, similar to global loopdepth

  }

  MetaData* clone() const final { return new AllAliasMetadata(*this); }

  std::vector<FunctionSignature> mightCall;
  bool shouldExport = true;
};

#endif  // CGCOLLECTOR2_ALLALIASMETADATA_H

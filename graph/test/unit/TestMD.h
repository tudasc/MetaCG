/**
 * File: TestMD.h
 * License: Part of the metacg project. Licensed under BSD 3 clause license. See LICENSE.txt file at
 * https://github.com/tudasc/metacg/LICENSE.txt
 */
#ifndef METACG_TESTMD_H
#define METACG_TESTMD_H

#include "io/VersionFourMCGReader.h"
#include "metadata/MetaData.h"

struct TestMetaDataV4 final : metacg::MetaData::Registrar<TestMetaDataV4> {
  static constexpr const char* key = "TestMetaDataV4";

  explicit TestMetaDataV4(const nlohmann::json& j, metacg::StrToNodeMapping&) {
    stored_int = j.at("stored_int");
    stored_double = j.at("stored_double");
    stored_string = j.at("stored_string");
  }

  explicit TestMetaDataV4(int storeInt, double storeDouble, const std::string& storeString) {
    stored_int = storeInt;
    stored_double = storeDouble;
    stored_string = storeString;
  }

 private:
  TestMetaDataV4(const TestMetaDataV4& other)
      : stored_int(other.stored_int), stored_double(other.stored_double), stored_string(other.stored_string) {}

 public:
  nlohmann::json toJson(metacg::NodeToStrMapping&) const final {
    return {{"stored_int", stored_int}, {"stored_double", stored_double}, {"stored_string", stored_string}};
  };

  const char* getKey() const override { return key; }

  void merge(const MetaData& toMerge, const metacg::MergeAction&, const metacg::GraphMapping&) final {
    if (std::strcmp(toMerge.getKey(), getKey()) != 0) {
      metacg::MCGLogger::instance().getErrConsole()->error(
          "The MetaData which was tried to merge with TestMetaDataV4 was of a different MetaData type");
      abort();
    }

    // const TestMetaDataV4* toMergeDerived = static_cast<const TestMetaDataV4*>(&toMerge);
  }

  std::unique_ptr<MetaData> clone() const final { return std::unique_ptr<TestMetaDataV4>(new TestMetaDataV4(*this)); }

  void applyMapping(const metacg::GraphMapping&) override {}

 private:
  int stored_int;
  double stored_double;
  std::string stored_string;
};

#endif  // METACG_TESTMD_H
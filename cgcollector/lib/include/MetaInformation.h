#ifndef CGCOLLECTOR_METAINFORMATION_H
#define CGCOLLECTOR_METAINFORMATION_H

#include "nlohmann/json.hpp"

class MetaInformation {
  // std::string name;

  // protected:
  // MetaInformation(std::string name) : name(name) {}

 public:
  virtual void applyOnJSON(nlohmann::json &json, const std::string &functionName, const std::string &metaFieldName) = 0;

  // std::string getName() {
  //  return name;
  //}
};

class NumberOfStatementsResult final : public MetaInformation {
 public:
  int numberOfStatements;

  void applyOnJSON(nlohmann::json &json, const std::string &functionName, const std::string &metaFieldName) override {
    json[functionName][metaFieldName] = numberOfStatements;
  };

  // NumberOfStatementsResult() : MetaInformation("NumberOfStatements") {}
};

#endif /* ifndef CGCOLLECTOR_METAINFORMATION_H */

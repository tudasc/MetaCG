/**
* File: SharedDefs.cpp
* License: Part of the MetaCG project. Licensed under BSD 3 clause license. See LICENSE.txt file at
* https://github.com/tudasc/metacg/LICENSE.txt
*/

#include "SharedDefs.h"
#include <nlohmann/json.hpp>

void to_json(nlohmann::json& j, const FunctionSignature& fs) {
  j = nlohmann::json{{"retType", fs.retType}, {"funcName", fs.possibleFuncNames}, {"paramTypes", fs.paramTypes}};
  ;
}

void from_json(const nlohmann::json& J, FunctionSignature& fs) {
  J.at("retType").get_to(fs.retType);
  J.at("funcName").get_to(fs.possibleFuncNames);
  J.at("paramTypes").get_to(fs.paramTypes);
}

std::ostream& operator<<(std::ostream& os, const FunctionSignature& fs) {
  os << fs.retType << " ";
  for (const auto& elem : fs.possibleFuncNames) {
    os << elem << "|";
  }
  os << '(';
  for (const auto& elem : fs.paramTypes) {
    os << elem << ",";
  }
  os << ")";
  return os;
}
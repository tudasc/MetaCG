/**
* File: SharedDefs.h
* License: Part of the MetaCG project. Licensed under BSD 3 clause license. See LICENSE.txt file at
* https://github.com/tudasc/metacg/LICENSE.txt
*/
#ifndef CGCOLLECTOR2_SHAREDDEFS_H
#define CGCOLLECTOR2_SHAREDDEFS_H

#include <nlohmann/json.hpp>

#include <string>
#include <vector>

enum AliasAnalysisLevel { No, All };

struct FunctionSignature {
  std::string retType;
  std::vector<std::string> possibleFuncNames;
  std::vector<std::string> paramTypes;
  bool operator==(const FunctionSignature& other) const{
    return retType==other.retType &&possibleFuncNames==other.possibleFuncNames && paramTypes==paramTypes;
  }

};

struct Plugin;

typedef std::vector<Plugin*> MetaCollectorVector;

std::ostream& operator<<(std::ostream& os, const FunctionSignature& fs);

namespace std {
template <typename T>
struct hash<std::vector<T>> {
  size_t operator()(const std::vector<T> v) const {
    size_t ret = 0;
    for (auto& e : v) {
      ret |= std::hash<T>()(e);
    }
    return ret;
  }
};

template <>
struct hash<FunctionSignature> {
  size_t operator()(const FunctionSignature& fs) const {
    return hash<std::string>()(fs.retType) | hash<std::vector<std::string>>()(fs.paramTypes);
  }
};

template <>
struct equal_to<FunctionSignature> {
  bool operator()(const FunctionSignature& lhs, const FunctionSignature& rhs) const {
    return std::hash<FunctionSignature>()(lhs) == std::hash<FunctionSignature>()(rhs);
  }
};

}  // namespace std

void to_json(nlohmann::json& j, const FunctionSignature& fs);
void from_json(const nlohmann::json& j, FunctionSignature& fs);

template <typename T>
std::string col2str(const T& col, const std::string& prefix = "", const std::string& split = ",",
                    const std::string& suffix = "") {
  std::string ret = prefix;
  for (auto begin = col.begin(), end = --col.end(); begin != end; begin++) {
    ret += *begin;
    ret += split;
  }
  ret += col.back();
  ret += suffix;
  return ret;
}

#endif  // CGCOLLECTOR2_SHAREDDEFS_H

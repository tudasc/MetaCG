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

/// The type of a function, i.e. what a call through a function pointer must agree on to reach it
struct FunctionSignature {
  std::string retType;
  std::vector<std::string> paramTypes;
  /// The implicit object parameter of a non-static member function
  /// Empty for free functions, static member functions and member functions with an explicit object parameter
  std::string thisType;

  bool operator==(const FunctionSignature& other) const {
    return retType == other.retType && paramTypes == other.paramTypes && thisType == other.thisType;
  }
};
namespace cgcollector2 {
struct Plugin;
}

typedef std::vector<cgcollector2::Plugin*> MetaCollectorVector;


namespace std {
template <>
struct hash<FunctionSignature> {
  size_t operator()(const FunctionSignature& fs) const {
    const auto combine = [](size_t seed, const std::string& s) {
      return seed ^ (std::hash<std::string>()(s) + 0x9e3779b97f4a7c15ULL + (seed << 6) + (seed >> 2));
    };
    size_t ret = combine(0, fs.retType);
    ret = combine(ret, fs.thisType);
    for (const auto& param : fs.paramTypes) {
      ret = combine(ret, param);
    }
    return ret;
  }
};

}  // namespace std

inline void to_json(nlohmann::json& j, const FunctionSignature& fs) {
  j = nlohmann::json{{"retType", fs.retType}, {"paramTypes", fs.paramTypes}, {"thisType", fs.thisType}};
}

inline void from_json(const nlohmann::json& J, FunctionSignature& fs) {
  J.at("retType").get_to(fs.retType);
  J.at("paramTypes").get_to(fs.paramTypes);
  J.at("thisType").get_to(fs.thisType);
}

inline std::ostream& operator<<(std::ostream& os, const FunctionSignature& fs) {
  os << fs.retType << " ";
  if (!fs.thisType.empty()) {
    os << '[' << fs.thisType << "]::";
  }
  os << '(';
  for (const auto& elem : fs.paramTypes) {
    os << elem << ",";
  }
  os << ")";
  return os;
}

template <typename T>
std::string col2str(const T& col, const std::string& prefix = "", const std::string& split = ",",
                    const std::string& suffix = "") {
  std::string ret = prefix;
  for (auto begin = col.begin(), end = --col.end(); begin != end; ++begin) {
    ret += *begin;
    ret += split;
  }
  ret += col.back();
  ret += suffix;
  return ret;
}

#endif  // CGCOLLECTOR2_SHAREDDEFS_H

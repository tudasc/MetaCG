
#include "JSONManager.h"

#include <iostream>

int main(int argc, char **argv) {

  if (argc < 3) {
    std::cerr << "Too few argument.\nUsage: ./" << argv[0] << " groundtruth.mcg result.mcg" << std::endl;
    return -1;
  }

  nlohmann::json gt;
  nlohmann::json gen;

  FuncMapT gtFuncMap;
  FuncMapT genFuncMap;

  // Read the ground truth, i.e., expected content
  gt = buildFromJSONv2(gtFuncMap, argv[1], nullptr);
  // Read the generated result
  gen = buildFromJSONv2(genFuncMap, argv[2], nullptr);

  // Test for equality of result and groundtruth
  // Test same functions in both [set of keys equal]
  std::unordered_set<std::string> unfound;
  for (const auto &[k, v] : gtFuncMap) {
    if (genFuncMap.find(k) == genFuncMap.end()) {
      unfound.insert(k);
    }
  }
  if (!unfound.empty()) {
    std::cerr << "Unfound keys: " << unfound.size() << std::endl;
    return -2;
  }
  // Test that entries for keys are equal w.r.t. structure
  for (const auto &[k, v] : gtFuncMap) {
    const auto genValue = genFuncMap[k];
    if (!v.compareStructure(genValue)) {
      std::cerr << "Structural difference for " << k << std::endl;
      return -3;
    }
    // Test that meta information for the same keys are present and they are equal
    if (!v.compareMeta(genValue)) {
      std::cerr << "Meta difference for " << k << std::endl;
      return -4;
    }
  }

  return 0;
}

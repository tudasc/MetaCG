#include "JSONManager.h"

#include <iostream>

int main(int argc, char** argv) {
  if (argc < 3) {
    std::cerr << "Too few argument.\nUsage: ./" << argv[0] << " gt.json result.mcg" << std::endl;
    return -1;
  }

  nlohmann::json gt;

  FuncMapT genFuncMap;

  // Read the ground truth, i.e., expected content
  readIPCG(argv[1], gt);
  if (gt.is_null()) {
    std::cerr << "Ground truth is null." << std::endl;
    return -1;
  }
  // std::cout << gt.dump() << std::endl;
  //  Read the generated result
  buildFromJSONv2(genFuncMap, argv[2], nullptr);
  for (const auto& element : gt.items()) {
    const std::string name = element.key();
    const std::set<std::string> expected = element.value()["required"];
    const bool strict = element.value()["strict"];
    const std::set<std::string> blacklist = element.value()["blacklist"];
    const auto it = genFuncMap.find(name);
    if (it == genFuncMap.end()) {
      std::cerr << "Required function not found: " << name << std::endl;
      return -2;
    }
    if (!it->second.check_callees(expected, blacklist, strict)) {
      std::cerr << "Function " << name << " does not call the expected functions" << std::endl;
      return -3;
    }
  }

  return 0;
}

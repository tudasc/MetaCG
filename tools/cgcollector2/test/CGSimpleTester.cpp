#include "nlohmann/json.hpp"

#include <fstream>
#include <iostream>

int main(int argc, char** argv) {
  if (argc != 3) {
    std::cerr << "Usage: " << argv[0] << " groundtruth.json collector-result.ipcg" << std::endl;
    return -1;
  }

  std::cout << "Running test for " << argv[1] << " == " << argv[2] << std::endl;

  nlohmann::json groundTruth;
  {
    std::ifstream file(argv[1]);
    file >> groundTruth;
  }

  nlohmann::json collectorResult;
  {
    std::ifstream file(argv[2]);
    file >> collectorResult;
  }

  for (auto& elem : groundTruth.at("_CG")) {
    for (auto& member : elem) {
      if (member.is_array()) {
        std::sort(member.begin(), member.end());
      }
    }
  }

  for (auto& elem : collectorResult.at("_CG")) {
    for (auto& member : elem) {
      if (member.is_array()) {
        std::sort(member.begin(), member.end());
      }
    }
  }

  if (groundTruth.at("_CG") == collectorResult.at("_CG")) {
    std::cout << "Test success" << std::endl;
    return 0;
  } else {
    std::cerr << "Test failure" << std::endl;

    return 1;
  }
}

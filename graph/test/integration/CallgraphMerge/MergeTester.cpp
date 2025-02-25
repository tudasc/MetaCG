
#include "MCGManager.h"
#include "io/VersionTwoMCGReader.h"
#include "io/VersionTwoMCGWriter.h"

#include "metadata/BuiltinMD.h"

#include <filesystem>
#include <iostream>

bool check(nlohmann::json testGraph, nlohmann::json groundTruth) {
  for (auto& elem : groundTruth.at("_CG")) {
    for (auto& member : elem) {
      if (member.is_array()) {
        std::sort(member.begin(), member.end());
      }
    }
  }

  for (auto& elem : testGraph.at("_CG")) {
    for (auto& member : elem) {
      if (member.is_array()) {
        std::sort(member.begin(), member.end());
      }
    }
  }

  return groundTruth.at("_CG") == testGraph.at("_CG");
}

int main(int argc, char** argv) {
  if (argc != 5) {
    std::cerr << "Usage: " << argv[0] << " cg_a.ipcg cg_b.ipcg groundtruth.gtmcg result.ipcg" << std::endl;
    return -1;
  }

  std::cout << "Running test for " << argv[1] << " merged with " << argv[2] << " == " << argv[3] << std::endl;

  const std::string inputA(argv[1]);
  const std::string inputB(argv[2]);
  const std::string inputGroundTruth(argv[3]);
  const std::string outputFile(argv[4]);

  auto& mcgManager = metacg::graph::MCGManager::get();

  metacg::io::FileSource fsA(inputA);
  metacg::io::FileSource fsB(inputB);

  metacg::io::VersionTwoMetaCGReader mcgReaderA(fsA);
  metacg::io::VersionTwoMetaCGReader mcgReaderB(fsB);

  mcgManager.addToManagedGraphs("cg_a", mcgReaderA.read());
  mcgManager.addToManagedGraphs("cg_b", mcgReaderB.read());

  metacg::io::VersionTwoMCGWriter mcgWriter;
  mcgManager.mergeIntoActiveGraph();

  metacg::io::JsonSink jsonSink;
  mcgWriter.writeActiveGraph(jsonSink);

  nlohmann::json groundtruthJson;
  std::ifstream groundtruthFile(inputGroundTruth);
  groundtruthFile >> groundtruthJson;

  // If both are equal, we are done
  if (check(groundtruthJson, jsonSink.getJson())) {
    return 0;
  }
  // Keep file for inspection
  std::ofstream file;
  file.open(outputFile);
  file << jsonSink.getJson().dump(4);
  file.close();
  std::cout << "Test failure: Keeping wrong results for inspection" << std::endl;
  return 1;
}
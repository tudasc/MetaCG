
#include "metacg/io/NameMapping.h"
#include "metacg/io/VersionFourMCGReader.h"
#include "metacg/io/VersionFourMCGWriter.h"
#include "metacg/Callgraph.h"
#include "metacg/MergePolicy.h"
#include "metacg/metadata/CallTypeMD.h"
#include "metacg/metadata/OverrideMD.h"

#include <algorithm>
#include <fstream>
#include <iostream>
#include <string>

using namespace metacg;

void task(metacg::Callgraph* cg) {
  for (const auto& [nodePair, namedMD] : cg->getEdges()) {
    if (auto it = namedMD.find(metacg::CallTypeMD::key);
        it != namedMD.end()) {

      if (auto* callTypeMD =
              dynamic_cast<metacg::CallTypeMD*>(it->second.get())) {

        if (callTypeMD->callType == metacg::CallType::VIRTUAL) {
          const NodeId& calleeId = nodePair.second;
          const NodeId& callerId = nodePair.first;

          const auto callee = cg->getNode(calleeId);
          const auto caller = cg->getNode(callerId);

          const auto& calleeMD = callee->getMetaDataContainer();

          if (auto it2 = calleeMD.find(metacg::OverrideMD::key);
              it2 != calleeMD.end()) {

            if (auto* overrideMD =
                    dynamic_cast<metacg::OverrideMD*>(
                        it2->second.get())) {

              metacg::NameMapping m(*cg);

              for (const auto& override : overrideMD->overriddenBy) {
                const auto& node = cg->getNode(override);

                if (!cg->existsEdge(*caller, *node)) {
                  cg->addEdge(*caller, *node);
                }

                // TODO: Add the appropriate CallTypeMD
                // metadata to the newly created edge.
              }
            }
          }
        }
      }
    }
  }
}

// Compare two callgraphs, ignoring the ordering of arrays.
bool check(nlohmann::json testGraph,
           nlohmann::json groundTruth) {

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
    std::cerr
        << "Usage: " << argv[0]
        << " cg_a.gtmcg cg_b.gtmcg groundtruth.gtmcg result.gtmcg"
        << std::endl;

    return -1;
  }

  const std::string inputA(argv[1]);
  const std::string inputB(argv[2]);
  const std::string inputGroundTruth(argv[3]);
  const std::string outputFile(argv[4]);

  std::cout << "Running test for "
            << inputA << " merged with " << inputB
            << " == " << inputGroundTruth
            << std::endl;

  metacg::io::FileSource fsA(inputA);
  metacg::io::FileSource fsB(inputB);

  metacg::io::VersionFourMCGReader readerA(fsA);
  metacg::io::VersionFourMCGReader readerB(fsB);

  auto cgA = readerA.read();
  auto cgB = readerB.read();

  cgA->merge(*cgB, metacg::MergeByName{}, task);

  metacg::io::VersionFourMCGWriter writer;
  writer.setExportSorted(true);

  metacg::io::JsonSink jsonSink;
  writer.write(cgA.get(), jsonSink);

  nlohmann::json groundtruthJson;

  std::ifstream groundtruthFile(inputGroundTruth);

  if (!groundtruthFile) {
    std::cerr << "Error: Could not open ground-truth file: "
              << inputGroundTruth << std::endl;
    return -1;
  }

  try {
    groundtruthFile >> groundtruthJson;
  } catch (const nlohmann::json::exception& e) {
    std::cerr << "Error reading ground-truth JSON: "
              << e.what() << std::endl;
    return -1;
  }

  if (check(jsonSink.getJson(), groundtruthJson)) {
    return 0;
  }

  std::ofstream out(outputFile);

  if (!out) {
    std::cerr << "Error: Could not open output file: "
              << outputFile << std::endl;
    return -1;
  }

  out << jsonSink.getJson().dump(4) << std::endl;

  if (!out) {
    std::cerr << "Error writing output file: "
              << outputFile << std::endl;
    return -1;
  }

  std::cout << "Test failure: Keeping wrong results for inspection in "
            << outputFile << std::endl;

  return 1;
}

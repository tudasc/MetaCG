/**
 * File: CGDiff.cpp
 * License: Part of the MetaCG project. Licensed under BSD 3 clause license. See LICENSE.txt file at
 * https://github.com/tudasc/metacg/LICENSE.txt
 */

#include "CgNode.h"
#include "DiffFormatter.h"
#include "GlobalMDDiff.h"
#include "NodeSummary.h"
#include "NodeSummaryComparator.h"
#include "NodeSummaryHasher.h"
#include "io/MCGReader.h"
#include "io/NameMapping.h"
#include <cxxopts.hpp>

std::vector<std::unique_ptr<metacg::Diff>> compare(const metacg::Callgraph& mcgA, const metacg::Callgraph& mcgB,
                                                   ComparisonMode mode,
                                                   const std::unordered_set<std::string>& ignoredMdKeys = {}) {
  using Set = std::unordered_set<NodeSummary, NodeSummaryHasher, NodeSummaryComparator>;

  ComparisonMode nameOnlyMode = ComparisonMode::ignoreBody | ComparisonMode::ignoreEdges |
    ComparisonMode::ignoreMetadata | ComparisonMode::ignoreEdgeMetadata |
    ComparisonMode::ignoreGlobalMetadata;

  NodeSummaryHasher nameHasher{nameOnlyMode};
  NodeSummaryComparator nameComparator{nameOnlyMode};

  NodeSummaryComparator comparator{mode};

  metacg::NameMapping mappingA(mcgA);
  metacg::NameMapping mappingB(mcgB);

  auto collectNodeSummaries = [&](const auto& mcg, metacg::NameMapping mapping) -> Set {
    Set nodes(0, nameHasher, nameComparator);
    for (const auto& node : mcg.getNodes()) {
      std::unordered_set<std::string> calleeNames;

      auto callees = mcg.getCallees(*node);
      calleeNames.reserve(callees.size());

      std::transform(callees.begin(), callees.end(), std::inserter(calleeNames, calleeNames.end()),
                     [](const metacg::CgNode* callee) { return callee->getFunctionName(); });

      // Collect edge metadata
      std::unordered_map<std::string, std::unordered_set<std::string>> edgeMetaData;
      if (!hasFlag(mode, ignoreEdgeMetadata)) {
        for (const metacg::CgNode* callee : callees) {
          std::unordered_set<std::string> emd;
          for (auto& metadata : mcg.getAllEdgeMetaData(*node, *callee)) {
            std::string key = metadata.first;
            std::string value = metadata.second->toJson(mapping).dump(); 

            if (ignoredMdKeys.count(key)) {
              continue;
            }

            emd.insert(key + ":" + value);
          }
          edgeMetaData.emplace(callee->getFunctionName(), std::move(emd));
        }
      }

      // Node metadata
      std::unordered_set<std::string> nodeMetadata;
      if (!hasFlag(mode, ignoreMetadata)) {
        for (const auto& metadata : node->getMetaDataContainer()) {
          std::string key = metadata.first;
          auto json = metadata.second->toJson(mapping);

          if (json.is_object()) {
            for (auto& [subKey, subValue] : json.items()) {
              if (subValue.is_array()) {
                std::sort(subValue.begin(), subValue.end());
              }
            }
          }
          std::string value = json.dump(-1);
          if (ignoredMdKeys.count(key)) {
            continue;
          }

          nodeMetadata.insert(key + ":" + value);
        }
      }

      nodes.insert(NodeSummary(node->getFunctionName(), node->getHasBody(), std::move(calleeNames),
                               std::move(nodeMetadata), std::move(edgeMetaData)));
    }

    return nodes;
  };

  // Compare nodes
  Set nodesA(0, nameHasher, nameComparator);
  Set nodesB(0, nameHasher, nameComparator);
  nodesA = collectNodeSummaries(mcgA, mappingA);
  nodesB = collectNodeSummaries(mcgB, mappingB);

  std::vector<std::unique_ptr<metacg::Diff>> diffs;
  // Global metadata diff
  if (!hasFlag(ComparisonMode::ignoreGlobalMetadata, mode)) {
    for (const auto& [key, mdA] : mcgA.getMetaDataContainer()) {
      if (ignoredMdKeys.count(key)) {
        continue;
      }
      auto itB = mcgB.getMetaDataContainer().find(key);

      if (itB == mcgB.getMetaDataContainer().end()) {
        auto mdAasString = mdA->toJson(mappingA).dump(-1);
        diffs.emplace_back(std::make_unique<GlobalMDDiff>(GlobalMDDiff(key, mdAasString, "")));
      } else if (mdA->toJson(mappingB).dump(-1) != itB->second->toJson(mappingB).dump(-1)) {
        auto mdAasString = mdA->toJson(mappingA).dump(-1);
        auto mdBasString = itB->second->toJson(mappingB).dump(-1);
        diffs.emplace_back(std::make_unique<GlobalMDDiff>(GlobalMDDiff(key, mdAasString, mdBasString)));
      }
    }

    for (const auto& [key, mdB] : mcgB.getMetaDataContainer()) {
      if (ignoredMdKeys.count(key)) {
        continue;
      }

      auto itA = mcgA.getMetaDataContainer().find(key);
      if (itA == mcgA.getMetaDataContainer().end()) {
        auto mdBasString = mdB->toJson(mappingA).dump(-1);
        diffs.emplace_back(std::make_unique<GlobalMDDiff>(GlobalMDDiff{key, "", mdBasString}));
      }
    }
  }

  // Creating Diffs
  for (const auto& nA : nodesA) {
    auto itB = nodesB.find(nA);  // find nameA in B

    // node is completely missing
    if (itB == nodesB.end()) {
      diffs.emplace_back(std::make_unique<NodeDiff>(NodeDiff::onlyInA(nA)));
    } else if (!comparator(*itB, nA)) {  // check if nodes completely equal
      diffs.emplace_back(std::make_unique<NodeDiff>(createNodeDiff(nA, *itB, mode)));
      nodesB.erase(itB);
    } else {
      nodesB.erase(itB);
    }
  }

  for (const auto& nB : nodesB) {
    diffs.emplace_back(std::make_unique<NodeDiff>(NodeDiff::onlyInB(nB)));
  }

  return diffs;
}

int main(int argc, char** argv) {
  try {
    cxxopts::Options options("cgdiff",
                             "Compare two call-graphs.\n"
                             "Returns 0 if call-graphs are equal, 1 otherwise.\n");

    options.add_options()("ignore-edges", "ignore edges", cxxopts::value<bool>()->default_value("false"))(
        "ignore-hasBody", "ignore hasBody", cxxopts::value<bool>()->default_value("false"))(
        "ignore-md", "ignore node metadata", cxxopts::value<bool>()->default_value("false"))(
        "ignore-edge-md", "ignore edge metadata", cxxopts::value<bool>()->default_value("false"))(
        "ignore-global-md", "ignore global metadata", cxxopts::value<bool>()->default_value("false"))(
        "emit-diff-on-success", "emit json diff on success", cxxopts::value<bool>()->default_value("false"))(
        "ignore-md-key", "ignore specific metadata", cxxopts::value<std::vector<std::string>>())(
        "o,output", "output file for diff", cxxopts::value<std::string>())("h,help", "Print help");

    ComparisonMode mode = static_cast<ComparisonMode>(0);  // Start with 0
    std::vector<std::string> ignoring;

    auto result = options.parse(argc, argv);

    if (result.count("help")) {
      std::cout << options.help() << std::endl;
      return 0;
    }

    if (result["ignore-edges"].as<bool>()) {
      mode = mode | ComparisonMode::ignoreEdges;
      ignoring.push_back("edges");
    }

    if (result["ignore-hasBody"].as<bool>()) {
      mode = mode | ComparisonMode::ignoreBody;
      ignoring.push_back("body");
    }

    if (result["ignore-md"].as<bool>()) {
      mode = mode | ComparisonMode::ignoreMetadata;
      ignoring.push_back("metadata");
    }

    if (result["ignore-edge-md"].as<bool>()) {
      mode = mode | ComparisonMode::ignoreEdgeMetadata;
      ignoring.push_back("edge-metadata");
    }

    if (result["ignore-global-md"].as<bool>()) {
      mode = mode | ComparisonMode::ignoreGlobalMetadata;
      ignoring.push_back("global-metadata");
    }

    std::unordered_set<std::string> ignoredMdKeys;
    if (result.count("ignore-md-key")) {
      for (auto& k : result["ignore-md-key"].as<std::vector<std::string>>()) {
        ignoredMdKeys.insert(k);
        ignoring.push_back(k + "MD");
      }
    }

    auto unmatched = result.unmatched();
    if (unmatched.size() < 2) {
      std::cerr << "Usage: ./cgdiff [options] <cg1.json> <cg2.json>\n";
      return 1;
    }

    std::string cg1 = unmatched[0];
    std::string cg2 = unmatched[1];

    // Read Call-Graphs
    metacg::io::FileSource fs1(cg1);
    metacg::io::FileSource fs2(cg2);

    std::unique_ptr<metacg::io::MCGReader> r1 = metacg::io::createReader(fs1);
    std::unique_ptr<metacg::io::MCGReader> r2 = metacg::io::createReader(fs2);

    auto mcgA = r1->read();
    auto mcgB = r2->read();

    auto diffs = compare(*mcgA, *mcgB, mode, ignoredMdKeys);

    std::ostream* out = &std::cout;
    std::ofstream ofs;

    if ((!diffs.empty() || result["emit-diff-on-success"].as<bool>())) {
      if (result.count("output")) {
        ofs.open(result["output"].as<std::string>());
        if (!ofs) {
          std::cerr << "Error opening output file\n";
          return 1;
        }
        out = &ofs;
      }

      *out << DiffFormatter::emitAsJson(diffs, ignoring, std::filesystem::absolute(cg1).string(),
                                        std::filesystem::absolute(cg2).string())
        .dump();
    }

    if (diffs.empty()) {
      std::cout << "File are equal" << std::endl;
      return 0;
    } else {
      std::cout << "Call-graphs differ" << std::endl;
      return 1;
    }
  } catch (const cxxopts::exceptions::invalid_option_syntax& e) {
    std::cerr << "Error parsing options: " << e.what() << std::endl;
    return 2;
  } catch (const std::exception& e) {
    std::cerr << "Unexpected error: " << e.what() << std::endl;
    return 3;
  }
}

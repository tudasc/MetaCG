/**
 * File: CGDiff.cpp 
 * License: Part of the MetaCG project. Licensed under BSD 3 clause license. See LICENSE.txt file at
 * https://github.com/tudasc/metacg/LICENSE.txt
 */

#include "NodeSummary.h"
#include "NodeSummaryComparator.h"
#include "NodeSummaryHasher.h"
#include "io/MCGReader.h"
#include "io/NameMapping.h"
#include "CgNode.h"
#include <cxxopts.hpp>


std::vector<NodeDiff> compare(const metacg::Callgraph& mcgA,
        const metacg::Callgraph& mcgB, ComparisonMode mode) {
    using Set = std::unordered_set<NodeSummary, NodeSummaryHasher, NodeSummaryComparator>;

    ComparisonMode nameOnlyMode = ComparisonMode::ignoreBody | ComparisonMode::ignoreEdges | ComparisonMode::ignoreMetadata;

    NodeSummaryHasher nameHasher{nameOnlyMode};
    NodeSummaryComparator nameComparator{nameOnlyMode};

    NodeSummaryComparator comparator{mode};

    metacg::NameMapping mappingA(mcgA);
    metacg::NameMapping mappingB(mcgB);

    auto collectNodeSummaries = [&](const auto& mcg) -> Set {
        Set nodes(0, nameHasher, nameComparator);
        for (const auto& node : mcg.getNodes()) {

            std::unordered_set<std::string> calleeNames;

            auto callees = mcg.getCallees(*node);
            calleeNames.reserve(callees.size());

            std::transform(
                    callees.begin(), callees.end(),
                    std::inserter(calleeNames, calleeNames.end()),
                    [](const metacg::CgNode* callee) { return callee->getFunctionName(); }
                    );

            // Metadata
            if (!hasFlag(mode, ignoreMetadata)) {
                std::unordered_set<std::string> metadataList;
                for (const auto& metadata : node->getMetaDataContainer()) {
                    metadataList.insert(metadata.first + ":" + metadata.second->toJson(mappingA).dump(-1));
                }

                nodes.insert(NodeSummary(node->getFunctionName(), node->getHasBody(), 
                            std::move(calleeNames), std::move(metadataList)));
            } else {
                nodes.insert(NodeSummary(node->getFunctionName(), node->getHasBody(), 
                            std::move(calleeNames)));
            }
        }


        return nodes;

    };

    // Compare nodes
    Set nodesA(0, nameHasher, nameComparator);
    Set nodesB(0, nameHasher, nameComparator);
    nodesA = collectNodeSummaries(mcgA);
    nodesB = collectNodeSummaries(mcgB);
   
    // Creating Diffs
    std::vector<NodeDiff> diffs;
    for (const auto& nA : nodesA) {
        auto itB = nodesB.find(nA); // find nameA in B
                                   
        // node is completely missing 
        if (itB == nodesB.end()) {
            diffs.emplace_back(NodeDiff::onlyInA(nA));
        }
        else if (!comparator(*itB, nA)) { // check if nodes completely equal
            diffs.emplace_back(createNodeDiff(nA, *itB, mode)); 
            nodesB.erase(itB); 
        }
        else {
            nodesB.erase(itB); 
        }

    }

    for (const auto& nB : nodesB) {
        diffs.emplace_back(NodeDiff::onlyInB(nB));
    }

    return diffs;
}

int main(int argc, char** argv) {
    try {
        cxxopts::Options options("cgdiff", 
                "Compare different Call-Graphs.\n"
                "Returns 0 if call graphs are equal, 1 otherwise.\n");

        options.add_options()
            ("ignore-edges", "ignore Edges", cxxopts::value<bool>()->default_value("false"))
            ("ignore-body" , "ignore Body", cxxopts::value<bool>()->default_value("false"))
            ("ignore-md" , "ignore Metadata", cxxopts::value<bool>()->default_value("false"))
            ("emit-diff-as-json", "emit json diff file", cxxopts::value<bool>()->default_value("false"))
            ("emit-diff-as-text", "emit text diff file", cxxopts::value<bool>()->default_value("false"))
            ("o,output", "Output file for diff", cxxopts::value<std::string>())
            ("h,help" , "Print help")
            ;

        ComparisonMode mode = static_cast<ComparisonMode>(0);  // Start with 0
        std::vector<std::string> ignoring;

        auto result = options.parse(argc, argv);

        if (result.count("help")) {
            std::cout << options.help() << std::endl;
            exit(0);
        }

        if (result["ignore-edges"].as<bool>()) {
            mode = mode | ComparisonMode::ignoreEdges;
            ignoring.push_back("edges");
        }

        if (result["ignore-body"].as<bool>()) {
            mode = mode | ComparisonMode::ignoreBody;
            ignoring.push_back("body");
        }

        if (result["ignore-md"].as<bool>()) {
            mode = mode | ComparisonMode::ignoreMetadata;
            ignoring.push_back("metadata");
        }

        if (argc < 3) {
            std::cerr << "Usage: ./cgdiff [options] <cg1.json> <cg2.json>\n";
            return 1;
        }
        // positional arguments
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

        auto diffs = compare(*mcgA, *mcgB, mode);

        std::ostream* out = &std::cout;
        std::ofstream ofs;

        if (result.count("output")) {
            ofs.open(result["output"].as<std::string>());
            if (!ofs) {
                std::cerr << "Error opening output file\n";
                return 1;
            }
            out = &ofs;
        }

        if (result["emit-diff-as-json"].as<bool>()) {
            *out << NodeDiff::emitAsJson(diffs, ignoring, std::filesystem::absolute(cg1).string(), std::filesystem::absolute(cg2).string()).dump(-1);
        }

        if (result["emit-diff-as-text"].as<bool>()) {
            *out << NodeDiff::emitAsText(diffs, ignoring, std::filesystem::absolute(cg1).string(), std::filesystem::absolute(cg2).string());
        }

        return diffs.empty() ? 0 : 1;
    } catch (const cxxopts::exceptions::invalid_option_syntax& e) {
        std::cerr << "Error parsing options: " << e.what() << std::endl;
        return 1;
    } catch (const std::exception& e) {
        std::cerr << "Unexpected error: " << e.what() << std::endl;
        return 1;
    }
}

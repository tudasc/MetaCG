/**
 * File: CGDiff.cpp
 * License: Part of the MetaCG project. Licensed under BSD 3 clause license. See LICENSE.txt file at
 * https://github.com/tudasc/metacg/LICENSE.txt
 */

#pragma once

#include "NodeDiff.h"
#include "GlobalMDDiff.h"


struct DiffFormatter {
    /**
     * Emit json representation of a list of NodeDiffs.
     *
     * @param nodeDiffs Vector of NodeDiff objects to format
     * @param ignoring  List of difference categories that were ignored
     * @return Formatted string summarizing the differences
     */
    static nlohmann::ordered_json emitAsJson(
        const std::vector<std::unique_ptr<Diff>>& diffs,

        const std::vector<std::string>& ignoring,
        std::string_view cgA,
        std::string_view cgB)
    {
        nlohmann::ordered_json root;
        root["cgA"] = cgA;
        root["cgB"] = cgB;
        root["ignoring"] = ignoring;

        // collect node diffs and global metadata diffs separately
        nlohmann::ordered_json nodeDiffs = nlohmann::ordered_json::object();
        nlohmann::ordered_json globalMetaDiffs = nlohmann::ordered_json::object();

        for (const auto &diff : diffs) {
            nlohmann::ordered_json inner = diff->toJson(); // each Diff subclass implements its own toJson()

            switch (diff->kind()) {
                case Diff::Kind::Node: {
                    auto nd = static_cast<const NodeDiff *>(diff.get());
                    nodeDiffs[nd->name] = inner;
                    break;
                }
                case Diff::Kind::GlobalMD: {
                    auto md = static_cast<const GlobalMDDiff *>(diff.get());
                    globalMetaDiffs[md->key] = inner;
                    break;
                }
            }
        }

        // attach them to the root "diff" object
        nlohmann::ordered_json diffObj;
        diffObj["nodeDiffs"] = nodeDiffs;
        diffObj["globalMetaDiff"] = globalMetaDiffs;

        root["diff"] = diffObj;
        return root;
    }
};
/**
 * Create a NodeDiff representing the differences between two nodes.
 *
 * Comparison can be customized using the `mode` flags to ignore specific parts.
 *
 * @param nsA NodeSummary from the first graph
 * @param nsB NodeSummary from the second graph
 * @param mode ComparisonMode flags controlling which differences to ignore
 * @return NodeDiff describing the differences between nsA and nsB
 */
NodeDiff createNodeDiff(const NodeSummary& nsA, const NodeSummary& nsB, ComparisonMode mode) {
    NodeDiff diff;
    diff.name = nsA.name;


    if (!hasFlag(mode, ignoreBody) && nsA.hasBody != nsB.hasBody) {
        diff.diffType.push_back("differentBody");
    }

    if (!hasFlag(mode, ignoreEdges)) {

        std::unordered_set<std::string> allCallees = nsA.callees;
        allCallees.insert(nsB.callees.begin(), nsB.callees.end());

        bool edgeMetadataDifferent = !hasFlag(mode, ignoreEdgeMetadata) &&
            nsA.edgeMetadata != nsB.edgeMetadata;

        for (const auto& callee : allCallees) {
            NodeDiff::EdgeDiff edgeDiff;
            edgeDiff.callee = callee;

            if (!nsB.callees.count(callee)) {
                edgeDiff.onlyInA = true;
            }
            if (!nsA.callees.count(callee)) {
                edgeDiff.onlyInB = true;
            }

            // edge metadata differences
            if (edgeMetadataDifferent) {
                auto itA = nsA.edgeMetadata.find(callee);
                auto itB = nsB.edgeMetadata.find(callee);

                if (itA != nsA.edgeMetadata.end() && itB != nsB.edgeMetadata.end()) {
                    for (const auto& md : itA->second) {
                        if (!itB->second.count(md)) {
                            edgeDiff.metadataOnlyInA.insert(md);
                        }	

                    }
                    for (const auto& md : itB->second) {
                        if (!itA->second.count(md)) {
                            edgeDiff.metadataOnlyInB.insert(md);
                        }
                    }
                } else if (itA != nsA.edgeMetadata.end() && itB == nsB.edgeMetadata.end()) {
                    edgeDiff.metadataOnlyInA = itA->second;
                } else if (itB != nsB.edgeMetadata.end() && itA == nsA.edgeMetadata.end()) {
                    edgeDiff.metadataOnlyInB = itB->second;
                }
            }

            // only add if there is a difference
            if (edgeDiff.onlyInA || edgeDiff.onlyInB || !edgeDiff.metadataOnlyInA.empty() || !edgeDiff.metadataOnlyInB.empty()) {
                diff.edgeDiffs.push_back(std::move(edgeDiff));
            }		
        }
        bool hasStructuralEdgeDiff = std::any_of(
            diff.edgeDiffs.begin(), diff.edgeDiffs.end(),
            [](const NodeDiff::EdgeDiff& ed) {
                return ed.onlyInA || ed.onlyInB;
            });
        bool hasEdgeMetadataDiff = std::any_of(
            diff.edgeDiffs.begin(), diff.edgeDiffs.end(),
            [](const NodeDiff::EdgeDiff& ed) {
                return !ed.metadataOnlyInA.empty() || !ed.metadataOnlyInB.empty();
            });

        if (hasStructuralEdgeDiff && !hasFlag(mode, ignoreEdges)) {
            diff.diffType.push_back("differentEdges");
        }

        if (hasEdgeMetadataDiff && !hasFlag(mode, ignoreEdgeMetadata)) {
            diff.diffType.push_back("differentEdgeMetadata");
        }
    }

    if (!hasFlag(mode, ignoreMetadata) && nsA.metadata != nsB.metadata) {
        diff.diffType.push_back("differentMetadata");

        for (const auto& md : nsA.metadata) {
            if (!nsB.metadata.count(md)) {
                diff.metadataOnlyInA.insert(md);
            }
        }

        for (const auto& md : nsB.metadata) {
            if (!nsA.metadata.count(md)) {
                diff.metadataOnlyInB.insert(md);
            }
        }
    }

    return diff;
}

/**
 * File: NodeSummary.h
 * License: Part of the MetaCG project. Licensed under BSD 3 clause license. See LICENSE.txt file at
 * https://github.com/tudasc/metacg/LICENSE.txt
 */
#pragma once
#include <Callgraph.h>
#include "nlohmann/json.hpp"
#include <string>

/**
 * Summary of a node in a call graph for comparison purposes.
 */
struct NodeSummary {
    std::string name;
    bool hasBody;
    std::unordered_set<std::string> callees; // use function names as unique identifier
    std::unordered_set<std::string> metadata; // use strings instead of json to make non-order-dependant comparison easier

    std::unordered_map<std::string, std::unordered_set<std::string>> edgeMetadata; // map callee -> metadata

    template<typename S>
    explicit NodeSummary(S&& name, bool hasBody, std::unordered_set<std::string> callees = {}, std::unordered_set<std::string> metadata = {}, 
                         std::unordered_map<std::string, std::unordered_set<std::string>> edgeMetadata = {}) 
        : name(std::forward<S>(name)), hasBody(hasBody), callees(std::move(callees)), metadata(std::move(metadata)), edgeMetadata(std::move(edgeMetadata)) {}


    template<typename S>
    explicit NodeSummary(S&& name, bool hasBody, std::unordered_set<std::string> callees) 
    : name(std::forward<S>(name)), hasBody(hasBody), callees(std::move(callees)) {}
};

/**
 * Flags controlling what parts of a call graph are ignored during comparison.
 *
 * Can be compined using bitwise OR to enable multiple options.
 */
enum ComparisonMode {
    ignoreEdges             = 1 << 0,
    ignoreBody              = 1 << 1,
    ignoreMetadata          = 1 << 2,
    ignoreEdgeMetadata      = 1 << 3,
    ignoreGlobalMetadata    = 1 << 4,
    none                    = 1 << 5
};

/**
 * Enable combining ComparisonMode flags with bitwise OR.
 */
inline ComparisonMode operator|(ComparisonMode a, ComparisonMode b) {
    return static_cast<ComparisonMode>(static_cast<int>(a) | static_cast<int>(b));
}

/**
 * Check whether a specific flag is set in a ComparisonMode bitmask.
 *
 * @param mode The combined ComparisonMode flags
 * @param flag The specific flag to check
 * @return true if the flag is set, false otherwise
 */
inline bool hasFlag(ComparisonMode mode, ComparisonMode flag) {
    return (static_cast<int>(mode) & static_cast<int>(flag)) != 0;
}

/**
 * Represents the differences between two call-graph nodes.
 *
 * Contains the node name, types of differences, and sets of callees
 * and metadata that exist in only one of the two compared nodes.
 */
struct NodeDiff {
    struct EdgeDiff {
        std::string callee;
        bool onlyInA = false;
        bool onlyInB = false;
        std::unordered_set<std::string> metadataOnlyInA;
        std::unordered_set<std::string> metadataOnlyInB;
    };

    std::string name;

    std::vector<std::string> diffType;

    std::vector<EdgeDiff> edgeDiffs;

    std::unordered_set<std::string> metadataOnlyInA;
    std::unordered_set<std::string> metadataOnlyInB;



    /**
     * Factory function for a node that exists only in the first graph (A).
     *
     * @param a NodeSummary from the first graph
     * @return NodeDiff representing a missing node in A
     */
    static NodeDiff onlyInA(const NodeSummary& a) {
        return NodeDiff{a.name, {"missingNode"}, {}, {}, {}};
    }

    /**
     * Factory function for a node that exists only in the first graph (B).
     *
     * @param a NodeSummary from the first graph
     * @return NodeDiff representing a missing node in B
     */
    static NodeDiff onlyInB(const NodeSummary& b) {
        return NodeDiff{b.name, {"missingNode"}, {}, {}, {}};
    }

    /**
     * Emit a textual representation of a list of NodeDiffs.
     *
     * @param nodeDiffs Vector of NodeDiff objects to format
     * @param ignoring  List of difference categories that were ignored
     * @return Formatted string summarizing the differences
     */
    static std::string emitAsText(const std::vector<NodeDiff>& nodeDiffs, std::vector<std::string> ignoring, std::string_view cgA, std::string_view cgB) {
        auto printList = [](const auto& container) -> std::string {
            std::ostringstream oss;
            oss << "[";
            for (auto it = container.begin(); it != container.end(); ++it) {
                oss << *it;
                if (std::next(it) != container.end()) oss << ", ";
            }
            oss << "]";
            return oss.str();
        };

        std::ostringstream os;

        os << "cgA : " << cgA << ", \n";
        os << "cgB : " << cgB << ", \n";
        os << "ignoring : " << printList(ignoring);

        for (const auto& nd : nodeDiffs) {
            os << nd.name << ":\n"
                << "\ttype : " << printList(nd.diffType) << ", \n"
                // << "\tcalleesOnlyInA : " << printList(nd.calleesOnlyInA) << ", \n"
                // << "\tcalleesOnlyInB : " << printList(nd.calleesOnlyInB) << ", \n"
                << "\tmetadataOnlyInA : " << printList(nd.metadataOnlyInA) << ", \n"
                << "\tmetadataOnlyInB : " << printList(nd.metadataOnlyInB) << "\n\n";
        }

        return os.str();
    }

    /**
     * Emit json representation of a list of NodeDiffs.
     *
     * @param nodeDiffs Vector of NodeDiff objects to format
     * @param ignoring  List of difference categories that were ignored
     * @return Formatted string summarizing the differences
     */
    static nlohmann::ordered_json emitAsJson(const std::vector<NodeDiff>& diffs,
                                             const std::vector<std::string>& ignoring,
                                             std::string_view cgA, std::string_view cgB) {
        nlohmann::ordered_json j;
        j["cgA"] = cgA;
        j["cgB"] = cgB;
        j["ignoring"] = ignoring;

        for (const auto& diff : diffs) {
            nlohmann::ordered_json inner;
            inner["diffType"] = diff.diffType;
            inner["metadataOnlyInA"] = diff.metadataOnlyInA;
            inner["metadataOnlyInB"] = diff.metadataOnlyInB;

            if (!diff.edgeDiffs.empty()) {
                nlohmann::ordered_json edgeArr = nlohmann::ordered_json::array();
                for (const auto& ed : diff.edgeDiffs) {
                    nlohmann::ordered_json edge;
                    edge["callee"] = ed.callee;
                    edge["onlyInA"] = ed.onlyInA;
                    edge["onlyInB"] = ed.onlyInB;
                    edge["metadataOnlyInA"] = ed.metadataOnlyInA;
                    edge["metadataOnlyInB"] = ed.metadataOnlyInB;
                    edgeArr.push_back(edge);
                }
                inner["edgeDiffs"] = edgeArr;
            }

            j[diff.name] = inner;
        }

        nlohmann::ordered_json root;
        root["diff"] = j;
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

    std::cout << "1" << std::endl;

    if (!hasFlag(mode, ignoreBody) && nsA.hasBody != nsB.hasBody) {
        diff.diffType.push_back("differentBody");
    }

    if (!hasFlag(mode, ignoreEdges)) {

        std::cout << "2" << std::endl;
        std::unordered_set<std::string> allCallees = nsA.callees;
        allCallees.insert(nsB.callees.begin(), nsB.callees.end());

        bool edgeMetadataDifferent = !hasFlag(mode, ignoreEdgeMetadata) &&
            nsA.edgeMetadata != nsB.edgeMetadata;

        std::cout << "creating NodeDiff: " << edgeMetadataDifferent << std::endl;
        for (const auto& callee : allCallees) {
            NodeDiff::EdgeDiff edgeDiff;
            edgeDiff.callee = callee;

            if (!nsB.callees.count(callee)) {
                edgeDiff.onlyInA = true;
            }
            if (!nsA.callees.count(callee)) {
                edgeDiff.onlyInB = true;
            }

            std::cout << "creating NodeDiff" << edgeMetadataDifferent << std::endl;
            // edge metadata differences
            if (edgeMetadataDifferent) {
                std::cout << "different edge md" << std::endl;
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

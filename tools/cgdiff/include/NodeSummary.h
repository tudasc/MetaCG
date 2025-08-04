/**
 * File: NodeSummary.h
 * License: Part of the MetaCG project. Licensed under BSD 3 clause license. See LICENSE.txt file at
 * https://github.com/tudasc/metacg/LICENSE.txt
 */
#pragma once
#include <string>
#include <Callgraph.h>
#include "nlohmann/json.hpp"

/**
 * Summary of a node in a call graph for comparison purposes.
 */
struct NodeSummary {
    std::string name;
    bool hasBody;
    std::unordered_set<std::string> callees; // use function names as unique identifier
    std::unordered_set<std::string> metadata; // use strings instead of json to make non-order-dependant comparison easier
    

    template<typename S>
    explicit NodeSummary(S&& name, bool hasBody, std::unordered_set<std::string> callees, std::unordered_set<std::string> metadata) 
        : name(std::forward<S>(name)), hasBody(hasBody), callees(std::move(callees)), metadata(std::move(metadata)) {}


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
    ignoreEdges     = 1 << 0,
    ignoreBody      = 1 << 1,
    ignoreMetadata  = 1 << 2,
    none            = 1 << 3
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
    std::string name;
    
    std::vector<std::string> diffType;

    std::unordered_set<std::string> calleesOnlyInA;
    std::unordered_set<std::string> calleesOnlyInB;

    std::unordered_set<std::string> metadataOnlyInA;
    std::unordered_set<std::string> metadataOnlyInB;

    /**
     * Factory function for a node that exists only in the first graph (A).
     *
     * @param a NodeSummary from the first graph
     * @return NodeDiff representing a missing node in A
     */
    static NodeDiff onlyInA(const NodeSummary& a) {
        return NodeDiff{a.name, {"missingNode"}, {}, {}, {}, {}};
    }

    /**
     * Factory function for a node that exists only in the first graph (B).
     *
     * @param a NodeSummary from the first graph
     * @return NodeDiff representing a missing node in B
     */
    static NodeDiff onlyInB(const NodeSummary& b) {
        return NodeDiff{b.name, {"missingNode"}, {}, {}, {}, {}};
    }

    /**
     * Emit a textual representation of a list of NodeDiffs.
     *
     * @param nodeDiffs Vector of NodeDiff objects to format
     * @param ignoring  List of difference categories that were ignored
     * @return Formatted string summarizing the differences
     */
    static std::string emitAsText(const std::vector<NodeDiff>& nodeDiffs, std::vector<std::string> ignoring) {
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

        for (const auto& nd : nodeDiffs) {
            os << nd.name << ":\n"
                << "\ttype : " << printList(nd.diffType) << ", \n"
                << "\tcalleesOnlyInA : " << printList(nd.calleesOnlyInA) << ", \n"
                << "\tcalleesOnlyInB : " << printList(nd.calleesOnlyInB) << ", \n"
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
    static nlohmann::json emitAsJson(std::vector<NodeDiff>& diffs, std::vector<std::string>& ignoring) {
        nlohmann::json j;                                                                                                                              

        j["ignoring"] = ignoring;                                                                                                                      

        for (const auto& diff : diffs) {                                                                                                               
            j.update(diff.toJson());                                                                                                                   
        }

        nlohmann::json root;                                                                                                                           
        root["diff"] = j;

        return root;
    }

    nlohmann::json toJson() const {
        nlohmann::json j;

        j["diffType"] = diffType;
        j["calleesOnlyInA"] = calleesOnlyInA;
        j["calleesOnlyInB"] = calleesOnlyInB;

        j["metadataOnlyInA"] = nlohmann::json::array();
        for (const auto& meta : metadataOnlyInA) j["metadataOnlyInA"].push_back(meta);

        j["metadataOnlyInB"] = nlohmann::json::array();
        for (const auto& meta : metadataOnlyInB) j["metadataOnlyInB"].push_back(meta);


        nlohmann::json res;
        res[name] = j;
        return res;
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

    if (!hasFlag(mode, ignoreEdges) && nsA.callees != nsB.callees) {
        diff.diffType.push_back("differentEdges");

        for (const auto& callee : nsA.callees) {
            if (!nsB.callees.count(callee)) {
                diff.calleesOnlyInA.insert(callee);
            }
        }
        for (const auto& callee : nsB.callees)
            if (!nsA.callees.count(callee))
                diff.calleesOnlyInB.insert(callee);
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

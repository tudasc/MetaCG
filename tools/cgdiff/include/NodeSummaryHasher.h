/**
 * File: NodeSummaryHasher.h 
 * License: Part of the MetaCG project. Licensed under BSD 3 clause license. See LICENSE.txt file at
 * https://github.com/tudasc/metacg/LICENSE.txt
 */

#pragma once
#include "NodeSummary.h"

/*
 * Provides a custom hash function for NodeSummary objects that respects
 * a ComparisonMode. Can be used with unordered containers (e.g., std::unordered_set)
 * to compare nodes based on specific fields while optionally ignoring others.
 *
 * The hash is order-independent for sets (callees, metadata) using XOR,
 * and combines individual field hashes using boost's hash-combine.
 */
class NodeSummaryHasher {
public:
    ComparisonMode mode;

    explicit NodeSummaryHasher(ComparisonMode mode) : mode(mode) {}

    size_t operator()(const NodeSummary& ns) const {
        size_t seed = 0;
        hash_combine(seed, ns.name);
        if (!hasFlag(mode, ignoreBody)) {
            hash_combine(seed, ns.hasBody);
        }
        if (!hasFlag(mode, ignoreEdges)) {
            // callees is unordered_set<string>
            for (const auto& callee : ns.callees) {
                // use XOR to create order-independant hash of callees
                seed ^= std::hash<std::string>{}(callee);
            }
        }
        if (!hasFlag(mode, ignoreMetadata)) {
            for (const auto& meta : ns.metadata) {
                seed ^= std::hash<std::string>{}(meta); // assumes that the hash for nlohmann::json is order-independant
            }
        }
        return seed;
    }

private:
    template <class T>
    static inline void hash_combine(std::size_t& seed, const T& v) {
        std::hash<T> hasher;
        seed ^= hasher(v) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
    }
};

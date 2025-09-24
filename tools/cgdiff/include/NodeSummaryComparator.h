/**
 * File: NodeSummaryComparator.h 
 * License: Part of the MetaCG project. Licensed under BSD 3 clause license. See LICENSE.txt file at
 * https://github.com/tudasc/metacg/LICENSE.txt
 */

#pragma once
#include "NodeSummary.h"

/**
 * Provides a custom comparison function for NodeSummary objects that respects
 * a ComparisonMode. Can be used with unordered containers or algorithms
 * that require equality checks based on selected fields.
 *
 * Only the fields not ignored by the ComparisonMode are considered.
 */
struct NodeSummaryComparator {
    ComparisonMode mode;

    explicit NodeSummaryComparator(ComparisonMode mode) : mode(mode) {}

    bool operator()(const NodeSummary& a, const NodeSummary& b) const {
        bool comparison = a.name == b.name;

        if (!hasFlag(mode, ignoreBody)) {
        }
        if (!hasFlag(mode, ignoreEdges)) {
            comparison &= a.callees == b.callees;
        }
        if (!hasFlag(mode, ignoreMetadata)) {
            comparison &= a.metadata == b.metadata;
        }
        if (!hasFlag(mode, ignoreEdgeMetadata) && !hasFlag(mode, ignoreEdges)) {
            comparison &= a.edgeMetadata == b.edgeMetadata;
        }

        return comparison;
    }
};



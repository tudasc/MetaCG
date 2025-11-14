/**
 * File: NameMapping.h
 * License: Part of the MetaCG project. Licensed under BSD 3 clause license. See LICENSE.txt file at
 * https://github.com/tudasc/metacg/LICENSE.txt
 */
#ifndef METACG_GRAPH_NAMEMAPPING_H
#define METACG_GRAPH_NAMEMAPPING_H

#include "io/IdMapping.h"
#include "Callgraph.h"

namespace metacg {
    struct NameMapping : metacg::NodeToStrMapping {
        public:
            explicit NameMapping(const metacg::Callgraph& graph) : graph(graph) {}

            virtual std::string getStrFromNode(metacg::NodeId id) override { return graph.getNode(id)->getFunctionName(); }

        private:
            const metacg::Callgraph& graph;
    };
}

#endif

/**
* File: CgTypes.h
* License: Part of the MetaCG project. Licensed under BSD 3 clause license. See LICENSE.txt file at
* https://github.com/tudasc/metacg/LICENSE.txt
*/
#ifndef METACG_GRAPH_CGNODEPTR_H
#define METACG_GRAPH_CGNODEPTR_H

#include <memory>
#include <unordered_set>

namespace metacg {
class CgNode;
class Callgraph;

using NodeId = size_t;

using CgNodePtr = std::unique_ptr<metacg::CgNode>;
using CgNodeRawPtrUSet = std::unordered_set<metacg::CgNode*>;

}  // namespace metacg



#endif
/**
 * File: IdMapping.h
 * License: Part of the MetaCG project. Licensed under BSD 3 clause license. See LICENSE.txt file at
 * https://github.com/tudasc/metacg/LICENSE.txt
 */
#ifndef METACG_GRAPH_IDMAPPING_H
#define METACG_GRAPH_IDMAPPING_H

#include "CgTypes.h"
#include <string>

namespace metacg {

struct CgNode;

/**
 * Maps string identifiers used in the json file to the respective nodes in the internal call graph representation.
 */
struct StrToNodeMapping {
  virtual CgNode* getNodeFromStr(const std::string& formatId) = 0;
};

/**
 * Maps nodes in the internal call graph representation to unique string identifiers used to identify nodes in the json
 * format.
 */
struct NodeToStrMapping {
  virtual std::string getStrFromNode(NodeId id) = 0;

  std::string getStrFromNode(const CgNode& node);
};

}  // namespace metacg

#endif  // METACG_GRAPH_IDMAPPING_H
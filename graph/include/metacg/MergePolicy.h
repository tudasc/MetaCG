/**
 * File: MergePolicy.h
 * License: Part of the MetaCG project. Licensed under BSD 3 clause license. See LICENSE.txt file at
 * https://github.com/tudasc/metacg/LICENSE.txt
 */
#ifndef METACG_MERGEPOLICY_H
#define METACG_MERGEPOLICY_H

#include <optional>

#include "metacg/CgTypes.h"

#include <unordered_map>
#include <vector>

namespace metacg {

class Callgraph;

/**
 * Represents the action of replacing or merging with a specific target node.
 */
struct MergeAction {
  MergeAction(NodeId targetNode, bool replace) : targetNode(targetNode), replace(replace) {}
  MergeAction() : MergeAction(-1, false) {}
  NodeId targetNode;
  bool replace;
};

using GraphMapping = std::unordered_map<NodeId, NodeId>;

struct MergeRecorder {
  /**
   * Records a merge of matching nodes.
   * @param sourceId Id of the node in the source graph.
   * @param action The executed action, as dictated by the chosen merge policy.
   */
  void recordMerge(NodeId sourceId, MergeAction action) {
    actions[sourceId] = action;
    mapping[sourceId] = action.targetNode;
  }

  /**
   * Records the copying of a node from the source into the destination graph, if there is no matching destination node.
   * @param sourceId Id of the node in the source graph.
   * @param targetId Id of the newly created node in the target graph.
   */
  void recordCopy(NodeId sourceId, NodeId targetId) { mapping[sourceId] = targetId; }

  /**
   * Returns the merge action performed on the given source node.
   * Can be used to determine the merging of metadata.
   * @param sourceId Id of the node in the source graph.
   * @return The performed merge action, if any.
   */
  std::optional<MergeAction> getAction(NodeId sourceId) const {
    if (auto it = actions.find(sourceId); it != actions.end()) {
      return it->second;
    }
    return {};
  }

  /**
   * Returns the graph mapping that resulted from this merge.
   * @return A mapping from the source to the destination graph.
   */
  const GraphMapping& getMapping() { return mapping; }

 private:
  std::unordered_map<NodeId, MergeAction> actions;
  GraphMapping mapping;
};

struct MergePolicy {
  virtual std::optional<MergeAction> findMatchingNode(const Callgraph& targetCG, const CgNode& sourceNode) const = 0;
  virtual ~MergePolicy() = default;
};

/**
 * This policy implements the behavior used by the old CGMerge.
 */
struct MergeByName : public MergePolicy {
  std::optional<MergeAction> findMatchingNode(const Callgraph& targetCG, const CgNode& sourceNode) const override;
  ~MergeByName() override = default;
};

/**
 * This policy matches nodes only if both their function name and their origin are identical.
 *
 * An exception is headers which match to source files if:
 *  1. Their origins have the same name and differ only in the file ending (very strict requirement).
 *  2. The sourcefile has a body and the header file does not have a body
 *
 * Nodes that share a name but differ in origin (e.g. static functions with the same name in different
 * translation units) are kept as separate nodes. Two nodes without an origin are treated as having the same origin.
 */
struct MergeByNameAndOrigin : public MergePolicy {
  std::optional<MergeAction> findMatchingNode(const Callgraph& targetCG, const CgNode& sourceNode) const override;
  ~MergeByNameAndOrigin() override = default;
};

/**
 * Returns the nodes in the target graph that have the same function name and the same origin as the source node.
 * If there are none, returns the nodes that are the header declaration / source definition counterpart of the source
 * node instead (see MergeByNameAndOrigin). Nodes with matching names but non-matching origins are filtered out.
 * Two nodes without an origin (std::nullopt) are considered to have the same origin.
 * This is the candidate search of MergeByNameAndOrigin, exposed so that custom merge policies can refine it.
 */
std::vector<const CgNode*> findOriginMatches(const Callgraph& targetCG, const CgNode& sourceNode);



}  // namespace metacg

#endif  // METACG_MERGEPOLICY_H

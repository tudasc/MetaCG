/**
* File: MergePolicy.h
* License: Part of the MetaCG project. Licensed under BSD 3 clause license. See LICENSE.txt file at
* https://github.com/tudasc/metacg/LICENSE.txt
*/
#ifndef METACG_MERGEPOLICY_H
#define METACG_MERGEPOLICY_H

#include <optional>

#include "CgTypes.h"

#include <unordered_map>

namespace metacg {

class Callgraph;

/**
 * Could be represented by a std::pair, but this makes the meaning more clear.
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
  void recordCopy(NodeId sourceId, NodeId targetId) {
    mapping[sourceId] = targetId;
  }

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
  const GraphMapping& getMapping() {
    return mapping;
  }

//  /**
//   * Returns the Id of the given node in the combined graph after merging.
//   * @param sourceId Id of the node in the source graph.
//   * @return Id of the node in the destination graph after merging.
//   */
//  std::optional<NodeId> getPostMergeId(NodeId sourceId) const {
//    if (auto it = mapping.find(sourceId); it != mapping.end()) {
//      return it->second;
//    }
//    return {};
//  }

 private:
  std::unordered_map<NodeId, MergeAction> actions;
  GraphMapping mapping;
};

struct MergePolicy {
  virtual std::optional<MergeAction> findMatchingNode(const Callgraph& targetCG, const CgNode& sourceNode) const = 0;
};

/**
 * This policy implements the behavior used by the old CGMerge.
 */
struct MergeByName : public MergePolicy {
  std::optional<MergeAction> findMatchingNode(const Callgraph& targetCG, const CgNode& sourceNode) const override;
};


}

#endif  // METACG_MERGEPOLICY_H

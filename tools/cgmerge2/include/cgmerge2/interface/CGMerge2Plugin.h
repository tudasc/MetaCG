/**
 * File: CGMerge2Plugin.h
 * License: Part of the MetaCG project. Licensed under BSD 3 clause license. See LICENSE.txt file at
 * https://github.com/tudasc/metacg/LICENSE.txt
 */

#ifndef CGMERGE2_PLUGIN_H
#define CGMERGE2_PLUGIN_H

/**
 * This is the base class for CGMerge2 plugins.
 * A plugin provides a user-defined merge policy: for every node of an input graph, CGMerge2 asks the plugin which
 * node of the merged graph it should be merged with, and whether the existing node should be replaced.
 * CGMerge2 loads exactly one plugin. If a plugin is loaded, it replaces the policy selected via --merge-policy.
 *
 * Thread safety: if CGMerge2 is built with OpenMP, findMatchingNode() is called concurrently from multiple threads
 * (on distinct target graphs). Implementations must therefore not modify shared state without synchronization.
 */

#include <metacg/MergePolicy.h>

#include <optional>
#include <string>

namespace metacg {
class Callgraph;
class CgNode;
}  // namespace metacg

namespace cgmerge2 {

struct Plugin : public metacg::MergePolicy {
  /**
   * Decides how a node of an input graph is merged into the target graph.
   * @param targetCG the (partially) merged graph that sourceNode is merged into
   * @param sourceNode the node of the input graph that is currently merged
   * @return the merge action to perform, i.e. the id of the node in targetCG to merge with and whether that node
   *         should be replaced by sourceNode. Return an empty optional to copy sourceNode into targetCG as a new node.
   */
  std::optional<metacg::MergeAction> findMatchingNode(const metacg::Callgraph& targetCG,
                                                      const metacg::CgNode& sourceNode) const override = 0;

  /**
   * Overwrite this if you want your plugin to be listed with a name in the logs
   * @return the logging name of your plugin
   */
  virtual std::string getPluginName() const { return "unnamed Plugin"; }

  ~Plugin() override = default;
};
}  // namespace cgmerge2

extern "C" {
// Needs to be implemented so the plugin can be loaded. CGMerge2 takes ownership of the returned object.
cgmerge2::Plugin* getCGMerge2Plugin();
}

#endif  // CGMERGE2_PLUGIN_H

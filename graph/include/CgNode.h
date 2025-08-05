/**
 * File: CgNode.h
 * License: Part of the MetaCG project. Licensed under BSD 3 clause license. See LICENSE.txt file at
 * https://github.com/tudasc/metacg/LICENSE.txt
 */
#ifndef METACG_GRAPH_CGNODE_H
#define METACG_GRAPH_CGNODE_H

// clang-format off

// Graph library
#include "CgTypes.h"
#include "metadata/MetaData.h"
#include "metadata/MetadataMixin.h"

// System library
#include <map>
#include <string>
#include <utility>
#include <memory>
#include <optional>
// clang-format on

namespace metacg {

class CgNode;
class Callgraph;

class CgNode : public MetadataMixin {
  friend class Callgraph;

 private:
  /**
   * Creates a call graph node for a function with name @function.
   * Cannot be invoked directly, use CallGraph::insert instead.
   * @param function
   */
  explicit CgNode(NodeId id, const std::string& function, std::optional<std::string> origin, bool isVirtual,
                  bool hasBody);

 public:
  ~CgNode() = default;

  /** We delete copy/move ctor, copy/move assign op */
  CgNode(const CgNode& other) = delete;
  CgNode(const CgNode&& other) = delete;
  CgNode& operator=(CgNode other) = delete;
  CgNode& operator=(const CgNode& other) = delete;
  CgNode& operator=(CgNode&& other) = delete;

  /**
   * Compares the function names for equality.
   * @param otherNode
   * @return
   */
  bool isSameFunctionName(const CgNode& otherNode) const { return functionName == otherNode.getFunctionName(); }

  /**
   * Check whether a function body has been found.
   * @param
   * @return
   */
  bool getHasBody() const { return hasBody; }

  /**
   * Mark function to have a body found
   * @param bodyStatus - the new status of the node
   * @return
   */
  void setHasBody(bool bodyStatus) { hasBody = bodyStatus; }

  /**
   * @return the nameIdMap of the function
   */
  std::string getFunctionName() const { return functionName; }

  /**
   * Sets the function name.
   * @param name
   */
  void setFunctionName(const std::string& name) { this->functionName = name; }

  [[deprecated("Attach \"OverrideMD\" instead")]] void setVirtual(bool);

  [[deprecated("Check with has<OverrideMD>() instead")]] bool isVirtual();

  /**
   * Returns the origin.
   * @return The origin string if set, or `std::nullopt`.
   */
  std::optional<std::string> getOrigin() const { return origin; }

  /**
   * Sets the origin path.
   * @param origin
   */
  void setOrigin(std::optional<std::string> origin) { this->origin = std::move(origin); }

  /**
   * Get the id of the function node
   *
   * @return the id of the function node
   */
  NodeId getId() const { return id; }

  friend std::ostream& operator<<(std::ostream& stream, const CgNode& n);

 public:
  const NodeId id;

 private:
  std::string functionName;
  std::optional<std::string> origin;
  bool hasBody;
  //  std::unordered_map<std::string, std::unique_ptr<MetaData>> metaFields;
};

}  // namespace metacg

#endif

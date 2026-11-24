/**
 * File: MergePolicy.cpp
 * License: Part of the MetaCG project. Licensed under BSD 3 clause license. See LICENSE.txt file at
 * https://github.com/tudasc/metacg/LICENSE.txt
 */
#include "metacg/MergePolicy.h"

#include "metacg/Callgraph.h"
#include "metacg/LoggerUtil.h"

namespace metacg {


std::optional<MergeAction> MergeByName::findMatchingNode(const Callgraph& targetCG, const CgNode& sourceNode) const {
  auto& nameMatches = targetCG.getAllNodesWithName(sourceNode.getFunctionName());
  if (nameMatches.empty()) {
    // There is no matching node: simply copy the source node into the target graph
    return {};
  }
  if (nameMatches.size() > 1) {
    // There is more than one node: in V2, this can't happen. Print a warning and merge with the first node.
    MCGLogger::logWarn("Found multiple nodes with matching name '{}'! Merging with the first node.",
                       sourceNode.getFunctionName());
  }
  auto* targetNode = targetCG.getNode(nameMatches.front());
  assert(targetNode && "Node must not be null");

  // If the source node has no body (i.e. no definition) OR the target node has one, keep the target node.
  // If both functions have bodies: assume inline with identical function bodies.
  if (targetNode->getHasBody() || !sourceNode.getHasBody()) {
    return MergeAction(targetNode->getId(), false);
  }

  return MergeAction(targetNode->getId(), true);
}


namespace {

/**
 * Checks whether two nodes with the same name but different origins belong to the same function, declared in a header
 * and defined in the corresponding source file. This is the case if one node has no body and originates from a header
 * (.h/.hpp), while the other node has a body and originates from a source file (.c/.cpp) with the same file name,
 * e.g. 'include/foo.h' and 'src/foo.cpp'.
 */
bool isHeaderSourcePair(const CgNode& targetNode, const CgNode& sourceNode) {
  if (!targetNode.getOrigin() || !sourceNode.getOrigin()) {
    return false;
  }
  const std::string targetOrigin = *targetNode.getOrigin();
  const std::string sourceOrigin = *sourceNode.getOrigin();

  // The file ending identifies headers and source files
  const std::string targetEnding = targetOrigin.substr(targetOrigin.find_last_of('.') + 1);
  const std::string sourceEnding = sourceOrigin.substr(sourceOrigin.find_last_of('.') + 1);
  const bool targetIsHeader = targetEnding == "h" || targetEnding == "hpp";
  const bool sourceIsHeader = sourceEnding == "h" || sourceEnding == "hpp";
  const bool targetIsImpl = targetEnding == "c" || targetEnding == "cpp";
  const bool sourceIsImpl = sourceEnding == "c" || sourceEnding == "cpp";
  if (!(targetIsHeader && sourceIsImpl) && !(targetIsImpl && sourceIsHeader)) {
    return false;
  }

  const auto& headerNode = targetIsHeader ? targetNode : sourceNode;
  const auto& implNode = targetIsHeader ? sourceNode : targetNode;

  const std::string targetFile = targetOrigin.substr(targetOrigin.find_last_of('/') + 1);
  const std::string sourceFile = sourceOrigin.substr(sourceOrigin.find_last_of('/') + 1);
  const bool fileNamesMatch =
      targetFile.substr(0, targetFile.find_last_of('.')) == sourceFile.substr(0, sourceFile.find_last_of('.'));

  if (headerNode.getHasBody() && implNode.getHasBody()) {
    if (fileNamesMatch) {
      MCGLogger::logWarn(
          "Function '{}' has a body in both '{}' and '{}'! Not considering them a match as their origins differ.",
          sourceNode.getFunctionName(), *headerNode.getOrigin(), *implNode.getOrigin());
    }
    return false;
  }

  if (headerNode.getHasBody() || !implNode.getHasBody()) {
    // Only a declaration in a header and a definition in a source file are considered a pair.
    return false;
  }

  if (!fileNamesMatch) {
    MCGLogger::logDebug("Considered '{}' ({}) as a match for '{}' ({}) but origin filenames do not align",
                        sourceNode.getFunctionName(), sourceOrigin, targetNode.getFunctionName(), targetOrigin);
    return false;
  }

  return true;
}

}  // namespace

std::optional<MergeAction> MergeByNameAndOrigin::findMatchingNode(const Callgraph& targetCG,
                                                                  const CgNode& sourceNode) const {
  auto& nameMatches = targetCG.getAllNodesWithName(sourceNode.getFunctionName());
  if (nameMatches.empty()) {
    // There is no matching node: simply copy the source node into the target graph
    return {};
  }

  // Among the nodes with a matching name, find those that also have the same origin.
  // Note: two nodes without an origin (std::nullopt) are considered to have the same origin.
  const auto sourceOrigin = sourceNode.getOrigin();
  CgNode* targetNode = nullptr;
  size_t numMatches = 0;
  for (const auto nodeId : nameMatches) {
    auto* candidate = targetCG.getNode(nodeId);
    assert(candidate && "Node must not be null");
    if (candidate->getOrigin() == sourceOrigin) {
      if (!targetNode) {
        targetNode = candidate;
      }
      ++numMatches;
    }
  }

  if (numMatches > 1) {
    MCGLogger::logWarn("Found multiple nodes with matching name '{}' and origin '{}'! Merging with the first node.",
                       sourceNode.getFunctionName(), sourceOrigin.value_or("<none>"));
  }

  if (!targetNode) {
    // No node with the same origin: check whether a node is the header declaration / source definition counterpart.
    for (const auto nodeId : nameMatches) {
      auto* candidate = targetCG.getNode(nodeId);
      assert(candidate && "Node must not be null");
      if (isHeaderSourcePair(*candidate, sourceNode)) {
        if (!targetNode) {
          targetNode = candidate;
        }
        ++numMatches;
      }
    }
    if (numMatches > 1) {
      MCGLogger::logWarn(
          "Found multiple header/source matches for '{}' with origin '{}'! Merging with the first node ('{}').",
          sourceNode.getFunctionName(), sourceOrigin.value_or("<none>"), targetNode->getOrigin().value_or("<none>"));
    }
  }

  if (!targetNode) {
    // Nodes with the same name exist, but none with a matching origin: copy the source node into the target graph
    return {};
  }

  // If the source node has no body (i.e. no definition) OR the target node has one, keep the target node.
  // If both functions have bodies: assume inline with identical function bodies.
  if (targetNode->getHasBody() || !sourceNode.getHasBody()) {
    return MergeAction(targetNode->getId(), false);
  }

  return MergeAction(targetNode->getId(), true);
}

}  // namespace metacg

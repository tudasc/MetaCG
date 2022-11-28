/**
 * File: CgNodePtr.h
 * License: Part of the MetaCG project. Licensed under BSD 3 clause license. See LICENSE.txt file at
 * https://github.com/tudasc/metacg/LICENSE.txt
 */
#ifndef METACG_GRAPH_CGNODEPTR_H
#define METACG_GRAPH_CGNODEPTR_H

#include <memory>
#include <unordered_set>

namespace metacg {
class CgNode;
}

/*
namespace std {
template <>
struct less<std::shared_ptr<metacg::CgNode>> {
  bool operator()(const std::shared_ptr<metacg::CgNode> &a, const std::shared_ptr<metacg::CgNode> &b) const;
};
template <>
struct less_equal<std::shared_ptr<metacg::CgNode>> {
  bool operator()(const std::shared_ptr<metacg::CgNode> &a, const std::shared_ptr<metacg::CgNode> &b) const;
};

template <>
struct equal_to<std::shared_ptr<metacg::CgNode>> {
  bool operator()(const std::shared_ptr<metacg::CgNode> &a, const std::shared_ptr<metacg::CgNode> &b) const;
};
template <>
struct greater<std::shared_ptr<metacg::CgNode>> {
  bool operator()(const std::shared_ptr<metacg::CgNode> &a, const std::shared_ptr<metacg::CgNode> &b) const;
};

template <>
struct greater_equal<std::shared_ptr<metacg::CgNode>> {
  bool operator()(const std::shared_ptr<metacg::CgNode> &a, const std::shared_ptr<metacg::CgNode> &b) const;
};
}  // namespace std
*/
typedef std::unique_ptr<metacg::CgNode> CgNodePtr;  // hopefully this typedef helps readability

typedef std::unordered_set<metacg::CgNode *> CgNodeRawPtrUSet;



#endif

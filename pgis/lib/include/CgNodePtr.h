/**
 * File: CgNodePtr.h
 * License: Part of the MetaCG project. Licensed under BSD 3 clause license. See LICENSE.txt file at
 * https://github.com/tudasc/metacg/LICENSE.txt
 */

#ifndef PIRA_CGNODE_PTR_H
#define PIRA_CGNODE_PTR_H

#include <memory>
#include <set>
#include <unordered_set>

class CgNode;

namespace std {
template <>
struct less<std::shared_ptr<CgNode>> {
  bool operator()(const std::shared_ptr<CgNode> &a, const std::shared_ptr<CgNode> &b) const;
};
template <>
struct less_equal<std::shared_ptr<CgNode>> {
  bool operator()(const std::shared_ptr<CgNode> &a, const std::shared_ptr<CgNode> &b) const;
};

template <>
struct equal_to<std::shared_ptr<CgNode>> {
  bool operator()(const std::shared_ptr<CgNode> &a, const std::shared_ptr<CgNode> &b) const;
};
template <>
struct greater<std::shared_ptr<CgNode>> {
  bool operator()(const std::shared_ptr<CgNode> &a, const std::shared_ptr<CgNode> &b) const;
};

template <>
struct greater_equal<std::shared_ptr<CgNode>> {
  bool operator()(const std::shared_ptr<CgNode> &a, const std::shared_ptr<CgNode> &b) const;
};
}  // namespace std

typedef std::shared_ptr<CgNode> CgNodePtr;  // hopefully this typedef helps readability

typedef std::set<CgNodePtr> CgNodePtrSet;
typedef std::unordered_set<CgNodePtr> CgNodePtrUnorderedSet;

#endif

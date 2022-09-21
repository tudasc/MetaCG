/**
 * File: CgNode.cpp
 * License: Part of the MetaCG project. Licensed under BSD 3 clause license. See LICENSE.txt file at https://github.com/tudasc/metacg/LICENSE.txt
 */

#include "CgNode.h"

namespace metacg {

CgNode::CgNode(std::string function)
    : functionName(std::move(function)),
      parentNodes(),
      childNodes(),
      isMarkedVirtual(false),
      hasBody(false) {
}

void CgNode::addChildNode(CgNodePtr childNode) { childNodes.insert(childNode); }

void CgNode::addParentNode(CgNodePtr parentNode) { parentNodes.insert(parentNode); }

void CgNode::removeChildNode(CgNodePtr childNode) { childNodes.erase(childNode);}

void CgNode::removeParentNode(CgNodePtr parentNode) { parentNodes.erase(parentNode); }

bool CgNode::isLeafNode() const { return getChildNodes().empty(); }
bool CgNode::isRootNode() const { return getParentNodes().empty(); }

bool CgNode::isSameFunction(CgNodePtr cgNodeToCompareTo) const {
  if (this->functionName.compare(cgNodeToCompareTo->getFunctionName()) == 0) {
    return true;
  }
  return false;
}

std::string CgNode::getFunctionName() const { return this->functionName; }

void CgNode::dumpToDot(std::ofstream &outStream, int procNum) {
#if 0
  std::shared_ptr<CgNode> thisNode;
  if (parentNodes.empty()) {
    for (auto childNode : childNodes) {
      for (int i = 0; i < procNum; i++) {
        std::string edgeColor = "";
        int numCalls = 0;
        if (isSpantreeParent(childNode)) {
          edgeColor = ", color=red, fontcolor=red";
        }

        for (auto parentNode : childNode->getParentNodes()) {
          if (this->functionName == parentNode->getFunctionName()) {
            thisNode = parentNode;
          }
        }

        outStream << "\"" << this->functionName << procNum << "\""
                  << " -> \"" << childNode->getFunctionName() << i << "\" [label=" << numCalls << edgeColor << "];"
                  << std::endl;
      }
    }
  } else {
    for (auto parentNode : parentNodes) {
      int numCalls = 0;
      if (!parentNode->get<BaseProfileData>()->getCgLocation().empty()) {
        std::string edgeColor = "";
        if (isSpantreeParent(parentNode)) {
          edgeColor = ", color=red, fontcolor=red";
        }

        for (auto cgLoc : this->get<BaseProfileData>()->getCgLocation()) {
          if (cgLoc.getProcId() == procNum) {
            numCalls += cgLoc.getNumCalls();
          }
        }

        outStream << "\"" << parentNode->getFunctionName() << procNum << "\""
                  << " -> \"" << this->functionName << procNum << "\" [label=" << numCalls << edgeColor << "];"
                  << std::endl;
      }
    }
  }
#endif
}

const CgNodePtrSet &CgNode::getChildNodes() const { return childNodes; }

const CgNodePtrSet &CgNode::getParentNodes() const { return parentNodes; }

void CgNode::instrumentFromParent(CgNodePtr parent) {
  this->instrumentedParentEdges.insert(parent);
}

void CgNode::print() {
#if 0
  std::cout << this->functionName << std::endl;
  for (auto n : childNodes) {
    std::cout << "--" << *n << std::endl;
  }
#endif
}

std::ostream &operator<<(std::ostream &stream, const CgNode &n) {
  stream << "\"" << n.getFunctionName() << "\"";

  return stream;
}

std::ostream &operator<<(std::ostream &stream, const CgNodePtrSet &s) {
  stream << "[ ";
  for (auto node : s) {
    stream << node->getFunctionName() << " | ";
  }
  stream << " ]";

  return stream;
}
}

namespace std {
bool less<std::shared_ptr<metacg::CgNode>>::operator()(const std::shared_ptr<metacg::CgNode> &a,
                                               const std::shared_ptr<metacg::CgNode> &b) const {
  return a->getFunctionName() < b->getFunctionName();
}

bool less_equal<std::shared_ptr<metacg::CgNode>>::operator()(const std::shared_ptr<metacg::CgNode> &a,
                                                     const std::shared_ptr<metacg::CgNode> &b) const {
  return a->getFunctionName() <= b->getFunctionName();
}

bool equal_to<std::shared_ptr<metacg::CgNode>>::operator()(const std::shared_ptr<metacg::CgNode> &a,
                                                   const std::shared_ptr<metacg::CgNode> &b) const {
  return a->getFunctionName() == b->getFunctionName();
}

bool greater<std::shared_ptr<metacg::CgNode>>::operator()(const std::shared_ptr<metacg::CgNode> &a,
                                                  const std::shared_ptr<metacg::CgNode> &b) const {
  return a->getFunctionName() > b->getFunctionName();
}

bool greater_equal<std::shared_ptr<metacg::CgNode>>::operator()(const std::shared_ptr<metacg::CgNode> &a,
                                                        const std::shared_ptr<metacg::CgNode> &b) const {
  return a->getFunctionName() >= b->getFunctionName();
}
}  // namespace std

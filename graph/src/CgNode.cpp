/**
 * File: CgNode.cpp
 * License: Part of the MetaCG project. Licensed under BSD 3 clause license. See LICENSE.txt file at
 * https://github.com/tudasc/metacg/LICENSE.txt
 */

#include "CgNode.h"

#include <iostream>
using namespace metacg;

std::string CgNode::getFunctionName() const { return functionName; }

bool CgNode::operator==(const CgNode &otherNode) const { return id == otherNode.id; }

bool CgNode::isSameFunction(const CgNode &otherNode) const { return id == otherNode.getId(); }

bool CgNode::isSameFunctionName(const CgNode &otherNode) const { return functionName == otherNode.getFunctionName(); }

void CgNode::dumpToDot(std::ofstream &outputStream, int procNum) {
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

CgNode::~CgNode() {
  for (const auto &md : metaFields) {
    delete md.second;
  }
}

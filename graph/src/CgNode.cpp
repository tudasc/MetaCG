/**
 * File: CgNode.cpp
 * License: Part of the MetaCG project. Licensed under BSD 3 clause license. See LICENSE.txt file at
 * https://github.com/tudasc/metacg/LICENSE.txt
 */

#include "CgNode.h"

#include <iostream>
using namespace metacg;

std::string CgNode::getFunctionName() const { return functionName; }

std::optional<std::string> CgNode::getOrigin() const { return origin; }

bool CgNode::isSameFunctionName(const CgNode& otherNode) const { return functionName == otherNode.getFunctionName(); }

std::ostream& operator<<(std::ostream& stream, const CgNode& n) {
  stream << "\"" << n.getFunctionName() << "\"";

  return stream;
}


/**
 * File: CgNode.cpp
 * License: Part of the MetaCG project. Licensed under BSD 3 clause license. See LICENSE.txt file at
 * https://github.com/tudasc/metacg/LICENSE.txt
 */

#include "CgNode.h"
#include "metadata/OverrideMD.h"

#include <iostream>
using namespace metacg;

CgNode::CgNode(NodeId id, const std::string& function, std::optional<std::string> origin, bool isVirtual, bool hasBody)
    : id(id), functionName(function), origin(std::move(origin)), hasBody(hasBody) {
  if (isVirtual) {
    this->getOrCreate<OverrideMD>();
  }
};

[[deprecated("Attach \"OverrideMD\" instead")]] void CgNode::setVirtual(bool virtualness) {
  if (virtualness) {
    this->getOrCreate<OverrideMD>();
  } else {
    this->metaFields.erase(OverrideMD::key);
  }
}

[[deprecated("Check with has<OverrideMD>() instead")]] bool CgNode::isVirtual() { return this->has<OverrideMD>(); }

std::ostream& operator<<(std::ostream& stream, const CgNode& n) {
  stream << "\"" << n.getFunctionName() << "\"";

  return stream;
}

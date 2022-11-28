/**
* File: CgEdge.cpp
* License: Part of the MetaCG project. Licensed under BSD 3 clause license. See LICENSE.txt file at
* https://github.com/tudasc/metacg/LICENSE.txt
*/

#include "CgEdge.h"
namespace metacg {
CgEdge::CgEdge(const metacg::CgNode &source,const metacg::CgNode &target) : source(source), target(target) {}

CgEdge::~CgEdge() {
  for (const auto &md : metaFields) {
    delete md.second;
  }
}
const metacg::CgNode &CgEdge::getSource() { return source; }

const metacg::CgNode &CgEdge::getTarget() { return target; }
}  // namespace metacg

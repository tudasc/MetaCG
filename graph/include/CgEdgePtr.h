/**
* File: CgEdgePtr.h
* License: Part of the MetaCG project. Licensed under BSD 3 clause license. See LICENSE.txt file at
* https://github.com/tudasc/metacg/LICENSE.txt
*/

#ifndef GRAPHLIB_CGEDGEPTR_H
#define GRAPHLIB_CGEDGEPTR_H

#include <memory>

namespace metacg {
class CgEdge;
}

typedef std::unique_ptr<metacg::CgEdge> CgEdgePtr;

#endif  // GRAPHLIB_CGEDGEPTR_H

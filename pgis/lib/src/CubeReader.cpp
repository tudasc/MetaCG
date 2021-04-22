/**
 * File: CubeReader.cpp
 * License: Part of the MetaCG project. Licensed under BSD 3 clause license. See LICENSE.txt file at https://github.com/tudasc/metacg/LICENSE.txt
 */

#include "CubeReader.h"
#include "CgNode.h"
#include "CgNodeMetaData.h"

#include "spdlog/spdlog.h"

using namespace pira;

void CubeCallgraphBuilder::build(std::string filepath, Config *c) { impl::build(filepath, impl::attRuntime); }

void CubeCallgraphBuilder::buildFromCube(std::string filePath, Config *c, CallgraphManager &cg) {
  // impl::build(filePath, impl::attRuntime, impl::attNrCall);
  impl::build(filePath, impl::attRuntime, impl::attNrCall, impl::attInclRuntime);
}

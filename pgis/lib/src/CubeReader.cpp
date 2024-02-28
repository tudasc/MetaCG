/**
 * File: CubeReader.cpp
 * License: Part of the metacg project. Licensed under BSD 3 clause license. See LICENSE.txt file at
 * https://github.com/tudasc/metacg/LICENSE.txt
 */

#include "CubeReader.h"
#include "../../../graph/include/CgNode.h"
#include "MetaData/CgNodeMetaData.h"

#include "PiraMCGProcessor.h"
#include "spdlog/spdlog.h"

using namespace pira;

void metacg::pgis::build(const std::filesystem::path &filepath, Config *c, metacg::graph::MCGManager &mcgm) {
  impl::build(filepath, mcgm, impl::attRuntime);
}

void metacg::pgis::buildFromCube(const std::filesystem::path &filePath, Config *c, metacg::graph::MCGManager &mcgm) {
  impl::build(filePath, mcgm, impl::attRuntime, impl::attNrCall, impl::attInclRuntime);
}

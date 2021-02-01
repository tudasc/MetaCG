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

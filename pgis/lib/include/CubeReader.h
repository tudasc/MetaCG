#ifndef CUBEREADER_H_
#define CUBEREADER_H_

#include "CallgraphManager.h"

#include <Cube.h>
#include <CubeMetric.h>

#include <string>

/**
 * \author roman
 * \author JPL
 */
namespace CubeCallgraphBuilder {

namespace impl {

template <typename N, typename... Largs>
void applyOne(cube::Cube &cu, cube::Cnode *cnode, N node, Largs... largs) {
  if constexpr (sizeof...(largs) > 0) {
    applyOne(cu, cnode, node, largs...);
  }
}
template <typename N, typename L, typename... Largs>
void applyOne(cube::Cube &cu, cube::Cnode *cnode, N node, L lam, Largs... largs) {
  lam(cu, cnode, node);
  applyOne(cu, cnode, node, largs...);
}
template <typename... Largs>
void apply(cube::Cube &cu, cube::Cnode *cnode, std::string &where, Largs... largs) {
  if constexpr (sizeof...(largs) > 0) {
    auto target = CallgraphManager::get().findOrCreateNode(where);
    applyOne(cu, cnode, target, largs...);
  }
}

void build(std::string filepath);
}  // namespace impl

void build(std::string filePath, Config *c);
void buildFromCube(std::string filePath, Config *c, CallgraphManager &cg);
}  // namespace CubeCallgraphBuilder

#endif

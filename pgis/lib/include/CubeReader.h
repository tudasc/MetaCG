#ifndef CUBEREADER_H_
#define CUBEREADER_H_

#include "CallgraphManager.h"
#include "CgNodeMetaData.h"

#include <Cube.h>
#include <CubeMetric.h>

#include <string>

/**
 * \author roman
 * \author JPL
 */
namespace CubeCallgraphBuilder {

namespace impl {

using BaseProfileData = pira::BaseProfileData;
using PiraOneData = pira::PiraOneData;
using PiraTwoData = pira::PiraTwoData;

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

template <typename T, typename U>
inline auto has(U u) {
  return u->template has<T>();
}
template <typename T, typename U>
inline auto get(U u) {
  return u->template get<T>();
}

const auto mangledName = [](const auto cn) { return cn->get_callee()->get_mangled_name(); };
const auto demangledName = [](const auto cn) { return cn->get_callee()->get_name(); };
const auto cMetric = [](std::string &&name, auto &&cube, auto cn) {
  if constexpr (std::is_pointer_v<decltype(cn)>) {
    typedef decltype(cube.get_sev(cube.get_met(name.c_str()), cn, cube.get_thrdv().at(0))) RetType;
    RetType metric{};
    for (auto t : cube.get_thrdv()) {
      metric += cube.get_sev(cube.get_met(name.c_str()), cn, t);
    }
    return metric;
  } else {
    assert(false);
  }
};
const auto time = [](auto &&cube, auto cn) { return cMetric(std::string("time"), cube, cn); };
const auto visits = [](auto &&cube, auto cn) { return cMetric(std::string("visits"), cube, cn); };

const auto cubeInclTime = [](auto &&cube, auto cn) {
  double inclusiveTime = 0;
  for (auto t : cube.get_thrdv()) {
    inclusiveTime += cube.get_sev(cube.get_met("time"), cube::CUBE_CALCULATE_INCLUSIVE, cn,
                                  cube::CUBE_CALCULATE_INCLUSIVE, t, cube::CUBE_CALCULATE_INCLUSIVE);
  }
  return inclusiveTime;
};

const auto getName = [](const bool mangled, const auto cn) {
  if (mangled) {
    return mangledName(cn);
  } else {
    return demangledName(cn);
  }
};

const auto attRuntime = [](auto &cube, auto cnode, auto n) {
  if (has<BaseProfileData>(n)) {
    spdlog::get("console")->info("Attaching runtime {} to node {}", impl::time(cube, cnode), n->getFunctionName());
    get<BaseProfileData>(n)->setRuntimeInSeconds(impl::time(cube, cnode));
  } else {
    spdlog::get("console")->warn("No BaseProfileData found for {}. This should not happen.", n->getFunctionName());
  }
  if (has<PiraOneData>(n)) {
    get<PiraOneData>(n)->setComesFromCube();
  }
};

const auto attNrCall = [](auto &cube, auto cnode, auto n) {
  if (has<BaseProfileData>(n)) {
    spdlog::get("console")->info("Attaching visits {} to node {}", impl::visits(cube, cnode), n->getFunctionName());
    get<BaseProfileData>(n)->setNumberOfCalls(impl::visits(cube, cnode));
  } else {
    spdlog::get("console")->warn("No BaseProfileData found for {}. This should not happen.", n->getFunctionName());
  }
};

const auto attInclRuntime = [](auto &cube, auto cnode, auto n) {
  if (has<BaseProfileData>(n)) {
    get<BaseProfileData>(n)->setInclusiveRuntimeInSeconds(cubeInclTime(cube, cnode));
  }
  if (has<PiraOneData>(n)) {
    get<PiraOneData>(n)->setComesFromCube();
  }
};

template <typename... Largs>
void build(std::string filePath, Largs... largs) {
  auto &cg = CallgraphManager::get();
  bool useMangledNames = true;

  auto console = spdlog::get("console");

  try {
    cube::Cube cube;
    // Read our cube file
    console->info("Reading cube file {}", filePath);
    cube.openCubeReport(filePath);
    // Get the cube nodes
    const auto &cnodes = cube.get_cnodev();

    console->trace("Cube contains: {} nodes", cnodes.size());
    for (const auto cnode : cnodes) {
      if (!cnode->get_parent()) {
        cg.findOrCreateNode(getName(useMangledNames, cnode), time(cube, cnode));
        continue;
      }

      auto pNode = cnode->get_parent();
      auto cNode = cnode;
      auto pName = getName(useMangledNames, pNode);
      auto cName = getName(useMangledNames, cNode);

      // Insert edge
      cg.putEdge(pName, cName);
      // Leave what to capture and attach to the user
      apply(cube, cnode, cName, largs...);
    }

  } catch (std::exception &e) {
    console->warn("Exception caught.\n{}", e.what());
  }
}

void build(std::string filepath);
}  // namespace impl

void build(std::string filePath, Config *c);
void buildFromCube(std::string filePath, Config *c, CallgraphManager &cg);
}  // namespace CubeCallgraphBuilder

#endif

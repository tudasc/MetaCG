/**
 * File: CubeReader.h
 * License: Part of the metacg project. Licensed under BSD 3 clause license. See LICENSE.txt file at
 * https://github.com/tudasc/metacg/LICENSE.txt
 */

#ifndef CUBEREADER_H_
#define CUBEREADER_H_

#include "LoggerUtil.h"
#include "MetaData/CgNodeMetaData.h"
#include "PiraMCGProcessor.h"

#include <Cube.h>
#include <CubeMetric.h>

#include <MCGManager.h>
#include <filesystem>
#include <string>

/**
 * \author roman
 * \author JPL
 */
namespace metacg::pgis {

namespace impl {

using BaseProfileData = pira::BaseProfileData;
using PiraOneData = pira::PiraOneData;
using PiraTwoData = pira::PiraTwoData;

template <typename N, typename... Largs>
void applyOne(cube::Cube& cu, [[maybe_unused]] cube::Cnode* cnode, [[maybe_unused]] N cgNode,
              [[maybe_unused]] cube::Cnode* pnode, [[maybe_unused]] N pCgNode, Largs... largs) {
  if constexpr (sizeof...(largs) > 0) {
    applyOne(cu, cnode, cgNode, pnode, pCgNode, largs...);
  }
}
template <typename N, typename L, typename... Largs>
void applyOne(cube::Cube& cu, [[maybe_unused]] cube::Cnode* cnode, [[maybe_unused]] N cgNode,
              [[maybe_unused]] cube::Cnode* pnode, [[maybe_unused]] N pCgNode, L lam, Largs... largs) {
  lam(cu, cnode, cgNode, pnode, pCgNode);
  applyOne(cu, cnode, cgNode, pnode, pCgNode, largs...);
}
template <typename... Largs>
void apply(metacg::graph::MCGManager& mcgm, [[maybe_unused]] cube::Cube& cu, [[maybe_unused]] cube::Cnode* cnode,
           const std::string& cNodeName, [[maybe_unused]] cube::Cnode* pnode, const std::string& pNodeName,
           Largs... largs) {
  if constexpr (sizeof...(largs) > 0) {
    auto target = &mcgm.getCallgraph()->getOrInsertNode(cNodeName);
    auto parent = mcgm.getCallgraph()->hasNode(pNodeName) ? &mcgm.getCallgraph()->getSingleNode(pNodeName) : nullptr;
    applyOne(cu, cnode, target, pnode, parent, largs...);
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
const auto cMetric = [](std::string&& name, auto&& cube, auto cn) {
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
const auto time = [](auto&& cube, auto cn) { return cMetric(std::string("time"), cube, cn); };
const auto visits = [](auto&& cube, auto cn) { return cMetric(std::string("visits"), cube, cn); };
const auto getName = [](const bool mangled, const auto cn) {
  if (mangled) {
    return mangledName(cn);
  } else {
    return demangledName(cn);
  }
};

const auto attRuntime = [](auto& cube, auto cnode, auto n, auto pNode, auto pn) {
  auto console = metacg::MCGLogger::instance().getConsole();
  if (has<BaseProfileData>(n)) {
    console->debug("Attaching runtime {} to node {}", impl::time(cube, cnode), n->getFunctionName());
    const auto runtime = impl::time(cube, cnode);
    const auto& bpd = get<BaseProfileData>(n);
    bpd->addRuntime(runtime);
  } else {
    console->warn("No BaseProfileData found for {}. This should not happen.", n->getFunctionName());
  }
  if (has<PiraOneData>(n)) {
    get<PiraOneData>(n)->setComesFromCube();
  }
};

const auto attNrCall = [](auto& cube, auto cnode, auto n, auto pNode, auto pn) {
  auto console = metacg::MCGLogger::instance().getConsole();
  if (has<BaseProfileData>(n)) {
    console->debug("Attaching visits {} to node {}", impl::visits(cube, cnode), n->getFunctionName());
    const auto calls = impl::visits(cube, cnode);
    const auto& bpd = get<BaseProfileData>(n);
    bpd->addCalls(calls);
    bpd->addNumberOfCallsFrom(pn, calls);
  } else {
    console->warn("No BaseProfileData found for {}. This should not happen.", n->getFunctionName());
  }
};

const auto attInclRuntime = [](auto& cube, auto cnode, auto n, [[maybe_unused]] auto pNode,
                               [[maybe_unused]] auto pName) {
  if (has<BaseProfileData>(n)) {
    // fill CgLocations and calculate inclusive runtime
    double cumulatedTime = 0;
    for (cube::Thread* thread : cube.get_thrdv()) {
      int threadId = thread->get_id();
      int procId = thread->get_parent()->get_id();

      unsigned long long numberOfCalls = (unsigned long long)cube.get_sev(cube.get_met("visits"), cnode, thread);

      double inclusiveTime = cube.get_sev(cube.get_met("time"), cube::CUBE_CALCULATE_INCLUSIVE, cnode,
                                          cube::CUBE_CALCULATE_INCLUSIVE, thread, cube::CUBE_CALCULATE_INCLUSIVE);

      double timeInSeconds = cube.get_sev(cube.get_met("time"), cnode, thread);
      // FIXME: Check if this also suffers from the cnode problem
      CgLocation cgLoc(timeInSeconds, inclusiveTime, threadId, procId, numberOfCalls);
      get<BaseProfileData>(n)->pushCgLocation(cgLoc);

      cumulatedTime += inclusiveTime;
    }

    get<BaseProfileData>(n)->addInclusiveRuntimeInSeconds(cumulatedTime);
    auto console = metacg::MCGLogger::instance().getConsole();
    console->debug("Attaching inclusive runtime {} to node {}", cumulatedTime, n->getFunctionName());
  } else if (has<PiraOneData>(n)) {
    get<PiraOneData>(n)->setComesFromCube();
  }
};

template <typename... Largs>
void build(const std::filesystem::path& filePath, metacg::graph::MCGManager& mcgm, Largs... largs) {
  //  auto &cg = metacg::pgis::PiraMCGProcessor::get();
  bool useMangledNames = true;

  auto console = metacg::MCGLogger::instance().getConsole();

  try {
    cube::Cube cube;
    // Read our cube file
    console->info("Reading cube file {}", filePath.string());
    cube.openCubeReport(filePath.string());
    // Get the cube nodes
    const auto& cnodes = cube.get_cnodev();

    console->trace("Cube contains: {} nodes", cnodes.size());
    for (const auto cnode : cnodes) {
      if (!cnode->get_parent()) {
        // Root node. This should be the name of the program and not main. Do not add it to the callgraph
        assert(getName(useMangledNames, cnode) != "main");
        continue;
      }

      auto pNode = cnode->get_parent();
      auto cNode = cnode;
      auto pName = getName(useMangledNames, pNode);
      auto cName = getName(useMangledNames, cNode);

      // Insert edge, if parent is not root
      if (pNode->get_parent()) {
        if (!mcgm.getCallgraph()->existsAnyEdge(pName, cName)) {
          mcgm.getCallgraph()->addEdge(pName, cName);
        } else {
          console->trace("Tried adding edge between {} and {} even though it already exists", pName, cName);
        }
      } else {
        assert(pName != "main");
      }

      // Leave what to capture and attach to the user
      apply(mcgm, cube, cnode, cName, pNode, pName, largs...);
    }

  } catch (std::exception& e) {
    console->warn("Exception caught.\n{}", e.what());
  }
}

void build(const std::filesystem::path& filepath);
}  // namespace impl

void build(const std::filesystem::path& filePath, Config* c, metacg::graph::MCGManager& mcgm);
void buildFromCube(const std::filesystem::path& filePath, Config* c, metacg::graph::MCGManager& mcgm);
}  // namespace metacg::pgis

#endif

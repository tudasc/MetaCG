/**
* File: MCGManager.cpp
* License: Part of the MetaCG project. Licensed under BSD 3 clause license. See LICENSE.txt file at
* https://github.com/tudasc/metacg/LICENSE.txt
 */


#include "MCGManager.h"

using namespace metacg::graph;

MCGManager &MCGManager::get() {
  static MCGManager instance;
  return instance;
}

void MCGManager::resetManager() {
  metaHandlers.clear();
  managedGraphs.clear();
  activeGraph = nullptr;
}

bool MCGManager::resetActiveGraph() {
  if (activeGraph) {
    activeGraph->clear();
    // just because a graph exists, doesn't mean it has attached metadata
    //  I think this is the best way to do it
    //  until map::contains comes with c++20
    //  I expect most callgraphs to have attached metadata
    //  so the exception should be rare
    try {
      metaHandlers.at(activeGraph).clear();
    } catch (const std::out_of_range &ex) {
    }
    return true;
  } else {
    assert(false && "Graph manager could not reset active Graph, no active graph exists");
    return false;
  }
}

std::vector<metacg::io::retriever::MetaDataHandler *> MCGManager::getMetaHandlers() const {
  std::vector<metacg::io::retriever::MetaDataHandler *> handler;
  // the tests expect an empty vector of metadata handlers to be returned if the active graph is either not metadata
  // annotated or doesn't exist
  if (metaHandlers.count(activeGraph) != 0) {
    for (const auto &mh : metaHandlers.at(activeGraph)) {
      handler.push_back(mh.get());
    }
  }
  return handler;
}

size_t MCGManager::size() const { return activeGraph->size(); }

size_t MCGManager::graphs_size() const { return managedGraphs.size(); }

metacg::Callgraph *MCGManager::getCallgraph() { return activeGraph; }

metacg::Callgraph *MCGManager::getOrCreateCallgraph(const std::string &name) { return managedGraphs[name].get(); }

bool MCGManager::setActive(const std::string &callgraph) {
  // I think this is the best way to do it
  // until map::contains comes with c++20
  // another implementation would be to
  // generate a new graph if no matching graph was found
  // I expect most users to only set a graph active
  // once it has been added, so the exception should be rare
  try {
    activeGraph = managedGraphs.at(callgraph).get();
  } catch (const std::out_of_range &ex) {
    assert(false && "Could not set graph to active, graph does not exist");
    return false;
  }
  return true;
}

bool MCGManager::assertActive(const std::string &callgraph) {
  try {
    return managedGraphs.at(callgraph).get() == activeGraph;
  } catch (const std::out_of_range &ex) {
    assert(false && "Graph is not part of managed graphs");
    return false;
  }
}

bool MCGManager::addToManagedGraphs(const std::string &name, std::unique_ptr<Callgraph> callgraph, bool setActive) {
  assert(callgraph.get() != nullptr && "Could not add to managed graphs, given graph was null");
  managedGraphs[name] = std::move(callgraph);
  if (setActive) {
    activeGraph = managedGraphs[name].get();
  }
  return true;
}
MCGManager::~MCGManager() = default;
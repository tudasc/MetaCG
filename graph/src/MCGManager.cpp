/**
 * File: MCGManager.cpp
 * License: Part of the MetaCG project. Licensed under BSD 3 clause license. See LICENSE.txt file at
 * https://github.com/tudasc/metacg/LICENSE.txt
 */

#include "MCGManager.h"
#include "LoggerUtil.h"

using namespace metacg::graph;

MCGManager& MCGManager::get() {
  static MCGManager instance;
  return instance;
}

void MCGManager::resetManager() {
  managedGraphs.clear();
  activeGraph = nullptr;
}

bool MCGManager::resetActiveGraph() {
  if (activeGraph) {
    activeGraph->clear();
    return true;
  }

  assert(false && "Graph manager could not reset active Graph, no active graph exists");
  return false;
}

size_t MCGManager::size() const { return activeGraph->size(); }

size_t MCGManager::graphs_size() const { return managedGraphs.size(); }

metacg::Callgraph* MCGManager::getCallgraph(const std::string& name, bool setActive) {
  if (name.empty()) {
    return activeGraph;
  }
  if (auto graph = managedGraphs.find(name); graph != managedGraphs.end()) {
    if (setActive) {
      this->setActive(name);
    }
    return graph->second.get();
  }
  return nullptr;
}

metacg::Callgraph* MCGManager::getOrCreateCallgraph(const std::string& name, bool setActive) {
  if (auto graph = managedGraphs.find(name); graph != managedGraphs.end()) {
    if (setActive) {
      activeGraph = graph->second.get();
    }
    return graph->second.get();
  }

  managedGraphs[name] = std::make_unique<Callgraph>();
  if (setActive) {
    activeGraph = managedGraphs[name].get();
  }
  return managedGraphs[name].get();
}

bool MCGManager::setActive(const std::string& callgraph) {
  // I think this is the best way to do it
  // until map::contains comes with c++20
  // another implementation would be to
  // generate a new graph if no matching graph was found
  // I expect most users to only set a graph active
  // once it has been added, so the exception should be rare
  try {
    activeGraph = managedGraphs.at(callgraph).get();
  } catch (const std::out_of_range& ex) {
    assert(false && "Could not set graph to active, graph does not exist");
    return false;
  }
  return true;
}

bool MCGManager::assertActive(const std::string& callgraph) {
  try {
    return managedGraphs.at(callgraph).get() == activeGraph;
  } catch (const std::out_of_range& ex) {
    assert(false && "Graph is not part of managed graphs");
    return false;
  }
}

bool MCGManager::addToManagedGraphs(const std::string& name, std::unique_ptr<Callgraph> callgraph, bool setActive) {
  assert(callgraph.get() != nullptr && "Could not add to managed graphs, given graph was null");
  managedGraphs[name] = std::move(callgraph);
  if (setActive) {
    activeGraph = managedGraphs[name].get();
  }
  return true;
}

void MCGManager::mergeIntoActiveGraph() {
  assert(activeGraph && "Graph manager could not merge into active Graph, no active graph exists");

  std::function<void(metacg::Callgraph*, metacg::Callgraph*, metacg::CgNode*)> copyNode =
      [&](metacg::Callgraph* destination, metacg::Callgraph* source, metacg::CgNode* node) {
        std::string functionName = node->getFunctionName();
        metacg::CgNode* mergeNode = destination->getOrInsertNode(functionName, node->getOrigin());

        if (node->getHasBody()) {
          auto callees = source->getCallees(node);

          for (auto* c : callees) {
            std::string calleeName = c->getFunctionName();
            if (!destination->hasNode(calleeName)) {
              copyNode(destination, source, c);
            }

            if (!destination->existEdgeFromTo(functionName, calleeName)) {
              destination->addEdge(functionName, calleeName);
            }
          }

          mergeNode->setHasBody(node->getHasBody());
          mergeNode->setIsVirtual(node->isVirtual());
        }

        for (const auto& it : node->getMetaDataContainer()) {
          if (mergeNode->has(it.first)) {
            mergeNode->get(it.first)->merge(*(it.second));
          } else {
            mergeNode->addMetaData(it.second->clone());
          }
        }
      };

  // We merge into whatever the active callgraph is
  auto* activeCallgraph = getCallgraph();
  for (const auto& callgraphName : getAllManagedGraphNames()) {
    if (assertActive(callgraphName))
      continue;

    auto* currentCallgraph = getCallgraph(callgraphName);

    for (auto it = currentCallgraph->getNodes().begin(); it != currentCallgraph->getNodes().end(); ++it) {
      auto& currentNode = it->second;
      copyNode(activeCallgraph, currentCallgraph, currentNode.get());
    }
  }
}

std::unordered_set<std::string> MCGManager::getAllManagedGraphNames() {
  std::unordered_set<std::string> retSet;
  retSet.reserve(managedGraphs.size());
  for (auto& elem : managedGraphs) {
    retSet.insert(elem.first);
  }
  return retSet;
}

std::string MCGManager::getActiveGraphName() {
  assert(activeGraph);
  for (const auto& elem : managedGraphs) {
    if (activeGraph == elem.second.get()) {
      return elem.first;
    }
  }
  assert(false);
  return "";
}

MCGManager::~MCGManager() = default;

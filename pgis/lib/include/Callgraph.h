#ifndef CALLGRAPH_H
#define CALLGRAPH_H

#include "CgHelper.h"
#include "CgNode.h"

class Callgraph {
 public:
  typedef CgNodePtrSet ContainerT;

  /**
   * @brief findMain
   * @return main fuction CgNodePtr
   */
  CgNodePtr findMain();

  /**
   * @brief findNode
   * @param functionName
   * @return the corresponding CgNodePtr or nullptr
   */
  CgNodePtr findNode(std::string functionName);

  void insert(CgNodePtr node);

  void eraseInstrumentedNode(CgNodePtr node);

  void erase(CgNodePtr node, bool rewireAfterDeletion = false, bool force = false);

  void clear() {
    graph.clear();
    lastSearched = nullptr;
    mainNode = nullptr;
  }

  ContainerT::iterator begin() const { return graph.begin(); }
  ContainerT::iterator end() const { return graph.end(); }
  ContainerT::const_iterator cbegin() const;
  ContainerT::const_iterator cend() const;

  bool hasNode(CgNodePtr n) { return graph.find(n) != graph.end(); }

  /**
   * @brief hasNode checks whether a node for #name exists in the graph mapping
   * @param name
   * @return true iff exists, false otherwise
   */
  bool hasNode(std::string name) {
    if (mainNode && name == "main") {
      lastSearched = mainNode;
      return true;
    }

    auto found = std::find_if(graph.begin(), graph.end(), [&](CgNodePtr n) { return n->getFunctionName() == name; });

    if (found != graph.end()) {
      lastSearched = *found;
      return true;
    }

    lastSearched = nullptr;
    return false;
  }

  /**
   * @brief getLastSearched - only call after hasNode returned True
   * @return node found by #hasNode - nullptr otherwise
   */
  CgNodePtr getLastSearched() { return lastSearched; }

  /**
   * @brief getNode searches the node in the graph and returns it
   * @param name
   * @return node for function with name - nullptr otherwise
   */
  CgNodePtr getNode(std::string name) {
    auto found = std::find_if(graph.begin(), graph.end(), [&](CgNodePtr n) { return n->getFunctionName() == name; });
    if (found != graph.end()) {
      return *found;
    }
    return nullptr;
  }

  size_t size();
  ContainerT &getGraph();

 private:
  // this set represents the call graph during the actual computation
  ContainerT graph;
  CgNodePtr mainNode;

  CgNodePtr lastSearched;
};

#endif

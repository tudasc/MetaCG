/**
 * File: MetaData.h
 * License: Part of the MetaCG project. Licensed under BSD 3 clause license. See LICENSE.txt file at
 * https://github.com/tudasc/metacg/LICENSE.txt
 */
#ifndef METACG_GRAPH_METADATA_H
#define METACG_GRAPH_METADATA_H

#include "CgNodePtr.h"

//#include "CallgraphManager.h"
#include "nlohmann/json.hpp"

namespace metacg {
namespace graph {
class MCGManager;
}

/**
 * This is the common base class for the different user-defined metadata.
 *  that can be attached to the call graph.
 *
 *  A class *must* implement a method static constexpr const char *key() that returns the class
 *  name as a string. This is used for registration in the MetaData field of the CgNode.
 */
struct MetaData {};

// TODO: Find better name for the namespace
namespace io::retriever {

using json = nlohmann::json;

/**
 * Example MetaRetriever
 *
 * A retriever needs to implement these three functions.
 */
struct MetaDataHandler {
  /** Invoked to decide if meta data should be output into the json file for the node */
  [[nodiscard]] virtual bool handles(const CgNodePtr n) const = 0;

  /** Invoked to find or create meta data entry in json */
  [[nodiscard]] virtual const std::string toolName() const = 0;

  /** Creates or returns the object to attach as meta information */
  virtual json value(const CgNodePtr n) const { return "Should not have happened"; }

  /** Reads the meta data from the json file and attaches it to the graph nodes */
  virtual void read([[maybe_unused]] const json &j, const std::string &functionName) = 0;

  /** Call back automatically invoked when adding to PiraMCGProcessor */
  void registerMCGManager(metacg::graph::MCGManager *manager) { mcgm = manager; }

  /** Reference to MCGManager (no owned) to interact while constructing */
  metacg::graph::MCGManager *mcgm;
};
}

}  // namespace metacg
#endif  // METACG_METADATA_H
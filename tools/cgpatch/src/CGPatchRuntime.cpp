/**
 * File: CGPatchRuntime.cpp
 * License: Part of the MetaCG project. Licensed under BSD 3 clause license. See LICENSE.txt file at
 * https://github.com/tudasc/metacg/LICENSE.txt
 */

#include "Callgraph.h"
#include "SymbolRetriever.h"
#include "io/VersionTwoMCGReader.h"
#include "io/VersionTwoMCGWriter.h"
#include "nlohmann/json.hpp"
#include <cstdlib>
#include <filesystem>
#include <iostream>

#if USE_MPI == 1
#include <mpi.h>
#endif
// applications do not require mpi.
#include <fstream>
#include <unordered_map>
#include <unordered_set>

using namespace SymbolRetriever;


namespace {
metacg::Callgraph* globalCallgraph;
MappedSymTableMap symTables;
bool shouldWrite = true;
int counter;

void initializeGlobalCallgraph() {
  if (metacg::graph::MCGManager& mcgManager = metacg::graph::MCGManager::get();
      mcgManager.getAllManagedGraphNames().empty()) {
    globalCallgraph = mcgManager.getOrCreateCallgraph("globalCallGraph", true);
    if (!globalCallgraph) {
      metacg::MCGLogger::logError("globalCallgraph is not initialized.");
      return;
    }
    counter = 0;
  }
}

void finalizeGlobalCallgraph() {
  // Write Callgraph to file
  // TODO: allow user to set format version
  metacg::graph::MCGManager& mcgManager = metacg::graph::MCGManager::get();
  if (shouldWrite) {
    metacg::io::VersionTwoMCGWriter mcgWriter;
    metacg::io::JsonSink jsonSink;
    mcgWriter.write(mcgManager.getCallgraph(), jsonSink);
    nlohmann::json j = jsonSink.getJson();

    const char* envVal = std::getenv("CGPATCH_CG_NAME");
    std::filesystem::path filename = envVal ? std::filesystem::path(envVal) : "validateGraph.json";

    if (filename.has_parent_path() && !std::filesystem::exists(filename.parent_path())) {
      metacg::MCGLogger::logError("Directory {} does not exist.", filename.parent_path().string());
      return;
    }

    std::ofstream ofs(filename);
    if (ofs.is_open()) {
      ofs << j;
      ofs.close();
    } else {
      metacg::MCGLogger::logError("Unable to open file {} for writing.", filename.string());
    }
  }
}

// Struct responsible for initialization and finalization of the patch-graph
struct ValidatorInitializer {
  ValidatorInitializer() {
    initializeGlobalCallgraph();
  }

  ValidatorInitializer(const ValidatorInitializer&) = delete;
  ValidatorInitializer& operator=(const ValidatorInitializer&) = delete;

  ValidatorInitializer(ValidatorInitializer&&) = delete;
  ValidatorInitializer& operator=(ValidatorInitializer&&) = delete;

  ~ValidatorInitializer() { finalizeGlobalCallgraph(); }
} _validator_init_finalize;
}  // namespace

extern "C" void __metacg_indirect_call(const char* name, void* address) {
  static std::unordered_map<const char*, std::unordered_set<void*>> visitedMap;
  if (!globalCallgraph) {
    metacg::MCGLogger::logError("globalCallgraph is not initialized.");
    return;
  }
  auto& knownCalls = visitedMap[name];
  if (knownCalls.find(address) != knownCalls.end()) {
    // We have seen this edge before
    return;
  }
  knownCalls.insert(address);

  // static ValidatorInitializer validator_init;
  initializeGlobalCallgraph();

  // resolve name
  // Loads symTables if symTables is not initialized yet. This potentially runs before the static constructor
  if (symTables.empty()) {
    symTables = loadMappedSymTables(getExecPath());
  }

  const std::string& symbol = findSymbol(reinterpret_cast<std::uintptr_t>(address), symTables);
  if (symbol.empty()) {
    metacg::MCGLogger::logError("Could not find symbol for address {:#x}", reinterpret_cast<std::uintptr_t>(address));
    return;
  }

  // Add new edge if edge does not exist yet
  if (!globalCallgraph->existEdgeFromTo(name, symbol)) {
    const auto caller = globalCallgraph->getOrInsertNode(name);
    const auto callee = globalCallgraph->getOrInsertNode(symbol);

    // set hasBody to true so the call-graphs can be fully merged
    caller->setHasBody(true);
    callee->setHasBody(true);

    globalCallgraph->addEdge(caller, callee);
    counter++;

    return;
  }
}

#if USE_MPI == 1

extern "C" int MPI_Finalize(void) {
  metacg::graph::MCGManager& mcgManager = metacg::graph::MCGManager::get();

  int rank, size;
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &size);

  if (rank != 0) {
    shouldWrite = false;
    // serialize call-graph
    metacg::io::VersionTwoMCGWriter mcgWriter;
    metacg::io::JsonSink jsonSink;
    mcgWriter.write(globalCallgraph, jsonSink);
    nlohmann::json j = jsonSink.getJson();

    // Send all call-graphs to rank 0
    std::string jsonStr = j.dump();
    MPI_Send(jsonStr.data(), jsonStr.size(), MPI_CHAR, 0, 0, MPI_COMM_WORLD);
  } else if (rank == 0) {
    shouldWrite = true;
    MPI_Status status;
    int msgSize;

    for (int i = 1; i < size; i++) {
      // Probe for incoming message
      MPI_Probe(i, 0, MPI_COMM_WORLD, &status);

      // Get size of incoming message
      MPI_Get_count(&status, MPI_CHAR, &msgSize);

      // Allocate buffer & receive message
      std::vector<char> buffer(msgSize);
      MPI_Recv(buffer.data(), msgSize, MPI_CHAR, i, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

      // Deserialize call-graph and add to mcgManager
      std::string jsonStr(buffer.begin(), buffer.end());
      nlohmann::json j = nlohmann::json::parse(jsonStr);
      metacg::io::JsonSource jsonSource(j);
      metacg::io::VersionTwoMetaCGReader mcgReader(jsonSource);

      mcgManager.addToManagedGraphs(std::to_string(i), std::move(mcgReader.read()), false);
    }

    mcgManager.mergeIntoActiveGraph();
  }
  return PMPI_Finalize();
}

#endif

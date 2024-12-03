/**
 * File: cgpatch-runtime.cpp
 * License: Part of the MetaCG project. Licensed under BSD 3 clause license. See LICENSE.txt file at
 * https://github.com/tudasc/metacg/LICENSE.txt
 */

#include "Callgraph.h"
#include "SymbolRetriever.h"
#include "io/VersionTwoMCGReader.h"
#include "io/VersionTwoMCGWriter.h"
#include "nlohmann/json.hpp"
#include <cstdlib>
#include <iostream>

#if USE_MPI == 1
#include <mpi.h>
#endif
// applications do not require mpi.
#include <fstream>
#include <llvm/Support/JSON.h>
#include <unordered_map>
#include <unordered_set>

using namespace SymbolRetriever;
namespace {
// Variables

metacg::Callgraph* globalCallgraph;
MappedSymTableMap symTables;
bool shouldWrite = true;
int counter;

// Logger
spdlog::logger* console;
spdlog::logger* errConsole;

void initializeGlobalCallgraph() {
  if (metacg::graph::MCGManager& mcgManager = metacg::graph::MCGManager::get();
      mcgManager.getAllManagedGraphNames().empty()) {
    globalCallgraph = mcgManager.getOrCreateCallgraph("globalCallGraph", true);
    if (!globalCallgraph) {
      errConsole->error("globalCallgraph is not initialized.");
      return;
    }
    console = metacg::MCGLogger::instance().getConsole();
    errConsole = metacg::MCGLogger::instance().getErrConsole();
    counter = 0;
  }
}

void finalizeGlobalCallgraph() {
  // Write Callgraph to file
  metacg::graph::MCGManager& mcgManager = metacg::graph::MCGManager::get();
  if (shouldWrite) {
    metacg::io::VersionTwoMCGWriter mcgWriter;
    metacg::io::JsonSink jsonSink;
    mcgWriter.write(mcgManager.getCallgraph(), jsonSink);
    nlohmann::json j = jsonSink.getJson();

    const char* filename = std::getenv("CGPATCH_CG_NAME");
    if (!filename) {
      filename = "validateGraph.json";
    }
    std::ofstream ofs(filename);
    if (ofs.is_open()) {
      ofs << j;
      ofs.close();
    } else {
      errConsole->error("Unable to open file {} for writing.", filename);
    }
  }
}

// Struct responsible for initialization and finalization of the patch-graph
struct ValidatorInitializer {
  ValidatorInitializer() { initializeGlobalCallgraph(); }

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
    errConsole->error("globalCallgraph is not initialized.");
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
  if (symTables.empty()) {  // Loads symTables if symTables is not initialized yet. This potentially runs before the
                            // static constructor
    symTables = loadMappedSymTables(getExecPath());
  }

  const std::string& symbol = findSymbol(reinterpret_cast<std::uintptr_t>(address), symTables);
  if (symbol.empty()) {
    errConsole->error("Could not find symbol for address {:#x}", reinterpret_cast<std::uintptr_t>(address));
    return;
  }

  // Add new edge if edge does not exist yet
  if (!globalCallgraph->existEdgeFromTo(name, symbol)) {
    const auto caller = globalCallgraph->getOrInsertNode(name);
    const auto callee = globalCallgraph->getOrInsertNode(symbol);

    // set hasBody to true so they call-graphs can be fully merged
    caller->setHasBody(true);
    callee->setHasBody(true);

    globalCallgraph->addEdge(caller, callee);
    counter++;

    return;
  }
}

#if USE_MPI == 1
// Overwrite MPI_Finalize using PMPI interface
/*
extern "C" int MPI_Comm_rank(void*, int*) __attribute__((weak));
extern "C" int MPI_Comm_size(void*, int*) __attribute__((weak));
extern "C" void* MPI_COMM_WORLD __attribute__((weak));
extern "C" int MPI_Barrier(void*) __attribute__((weak));
extern "C" int PMPI_Finalize(void) __attribute__((weak));
extern "C" int MPI_Abort(void*, int) __attribute__((weak));
*/

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
    std::string json_str = j.dump();
    MPI_Send(json_str.data(), json_str.size(), MPI_CHAR, 0, 0, MPI_COMM_WORLD);
  } else if (rank == 0) {
    shouldWrite = true;
    MPI_Status status;
    int msg_size;

    for (int i = 1; i < size; i++) {
      // Probe for incoming message
      MPI_Probe(i, 0, MPI_COMM_WORLD, &status);

      // Get size of incoming message
      MPI_Get_count(&status, MPI_CHAR, &msg_size);

      // Allocate buffer & receive message
      std::vector<char> buffer(msg_size);
      MPI_Recv(buffer.data(), msg_size, MPI_CHAR, i, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

      // Deserialize call-graph and add to mcgManager
      std::string json_str(buffer.begin(), buffer.end());
      nlohmann::json j = nlohmann::json::parse(json_str);
      metacg::io::JsonSource jsonSource(j);
      metacg::io::VersionTwoMetaCGReader mcgReader(jsonSource);

      mcgManager.addToManagedGraphs(std::to_string(i), std::move(mcgReader.read()), false);
    }

    mcgManager.mergeIntoActiveGraph();
  }

  return PMPI_Finalize();
}

#endif

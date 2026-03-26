/**
 * File: Main.cpp
 * License: Part of the MetaCG project. Licensed under BSD 3 clause license. See LICENSE.txt file at
 * https://github.com/tudasc/metacg/LICENSE.txt
 */

#include "ParseTreeVisitor.h"

#include <DotIO.h>
#include <llvm/Support/CommandLine.h>

using namespace metacg;
using namespace metacg::graph;
using namespace metacg::io;
using namespace metacg::cgfcollector;
using namespace Fortran::parser;

static MCGManager& mcgManager = MCGManager::get();

static llvm::cl::OptionCategory CGCategory("Callgraph Plugin Options");

static llvm::cl::opt<bool> Dot("dot", llvm::cl::desc("Generate DOT output"), llvm::cl::cat(CGCategory),
                               llvm::cl::init(false));
static llvm::cl::opt<bool> NoRename("no-rename", llvm::cl::desc("Do not rename output file"), llvm::cl::cat(CGCategory),
                                    llvm::cl::init(false));
static llvm::cl::opt<bool> Verbose("verbose", llvm::cl::desc("Enable verbose logging"), llvm::cl::cat(CGCategory),
                                   llvm::cl::init(false));
static llvm::cl::opt<std::string> graphName("graph-name",
                                            llvm::cl::desc("Name of the generated graph (default: \"cg\")"),
                                            llvm::cl::cat(CGCategory), llvm::cl::init("cg"));

/**
 * @brief Replace the file extension of filePath with newExtension. If filePath does not have an extension, append
 * newExtension.
 *
 * @param filePath
 * @param newExtension
 */
void replaceExtension(std::string& filePath, const std::string& newExtension) {
  size_t lastDotPos = filePath.find_last_of('.');
  if (lastDotPos != std::string::npos) {
    filePath = filePath.substr(0, lastDotPos) + newExtension;
  } else {
    filePath += newExtension;
  }
}

/**
 * @class CollectCG
 * @brief Plugin action to collect callgraph and dump it as JSON file
 *
 */
class CollectCG : public Fortran::frontend::PluginParseTreeAction {
 public:
  void executeAction() override {
    if (Verbose) {
      metacg::MCGLogger::instance().getConsole()->set_level(spdlog::level::debug);
      metacg::MCGLogger::instance().getConsole()->set_pattern("%v");
    }

    // create and register callgraph
    mcgManager.addToManagedGraphs(graphName, std::make_unique<metacg::Callgraph>(), true);

    Callgraph* cg = mcgManager.getCallgraph(graphName);
    if (!cg) {
      MCGLogger::logError("Failed to create callgraph");
      return;
    }

    // traverse parse tree and generate callgraph
    std::string currentFile = getCurrentFile().str();
    bool underscoring = getInstance().getInvocation().getLoweringOpts().getUnderscoring();

    ParseTreeVisitor visitor(cg, currentFile, underscoring);
    Fortran::parser::Walk(getParsing().parseTree(), visitor);
    visitor.postProcess();

    mcgManager.mergeIntoActiveGraph(metacg::MergeByName());

    // create writer
    auto mcgWriter = io::createWriter(4);
    if (!mcgWriter) {
      MCGLogger::logError("Unable to create a writer for format version {}", 4);
      return;
    }

    io::JsonSink jsonSink;
    mcgWriter->write(cg, jsonSink);

    // determine output file name. Honors `-o` option.
    std::string outputFile = getInstance().getFrontendOpts().outputFile;

    if (outputFile.empty()) {
      outputFile = getCurrentFile().str();
    }

    // Used for CMake managed tests. We override the Fortran compiler with our wrapper script. This generates call
    // graphs with the .mcg file extension. But CMake expects files with the .o extension. This option disables this
    // renaming.
    if (!NoRename) {
      replaceExtension(outputFile, ".mcg");
    }

    std::ofstream os(outputFile);
    os << jsonSink.getJson() << std::endl;

    // generate dot output
    if (Dot) {
      dot::DotGenerator dotGen(cg);
      dotGen.generate();

      auto dotOutputFile = outputFile;
      replaceExtension(dotOutputFile, ".dot");

      std::ofstream dotOs(dotOutputFile);
      dotOs << dotGen.getDotString() << std::endl;
    }
  }
};

static Fortran::frontend::FrontendPluginRegistry::Add<CollectCG> X("genCG", "Generate Callgraph");

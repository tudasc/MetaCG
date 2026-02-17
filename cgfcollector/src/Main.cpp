/**
 * File: Main.cpp
 * License: Part of the MetaCG project. Licensed under BSD 3 clause license. See LICENSE.txt file at
 * https://github.com/tudasc/metacg/LICENSE.txt
 */

#include "ParseTreeVisitor.h"

#include <DotIO.h>

using namespace metacg;
using namespace metacg::graph;
using namespace metacg::io;
using namespace Fortran::parser;

static MCGManager& mcgManager = MCGManager::get();

/**
 * @brief Create output file with given extension
 *
 * @param compInst
 * @param currentFile
 * @param extension
 * @return
 */
std::unique_ptr<llvm::raw_pwrite_stream> createOutputFile(Fortran::frontend::CompilerInstance& compInst,
                                                          llvm::StringRef currentFile, llvm::StringRef extension) {
  llvm::SmallString<128> outputPath(compInst.getFrontendOpts().outputFile);
  if (outputPath.empty()) {
    outputPath = currentFile;
  }
  if (extension != "")
    llvm::sys::path::replace_extension(outputPath, extension);
  std::unique_ptr<llvm::raw_fd_ostream> os;
  std::error_code ec;
  os.reset(new llvm::raw_fd_ostream(outputPath.str(), ec, llvm::sys::fs::OF_TextWithCRLF));
  if (ec) {
    MCGLogger::logError("Error opening output file: {}", ec.message());
    return nullptr;
  }
  return os;
}

/**
 * @brief Create callgraph in mcgManager and populate it by traversing the parse tree
 *
 * @param parseTree
 * @param currentFile
 */
void generateCG(std::optional<Program>& parseTree, llvm::StringRef currentFile) {
  mcgManager.addToManagedGraphs("cg", std::make_unique<metacg::Callgraph>(), true);
  Callgraph* cg = mcgManager.getCallgraph("cg");

#ifndef NDEBUG
  metacg::MCGLogger::instance().getConsole()->set_level(spdlog::level::debug);
  metacg::MCGLogger::instance().getConsole()->set_pattern("%v");
#endif

  ParseTreeVisitor visitor(cg, currentFile.str());
  Fortran::parser::Walk(parseTree, visitor);
  visitor.postProcess();

  mcgManager.mergeIntoActiveGraph(metacg::MergeByName());
}

/**
 * @brief Dump callgraph as JSON string
 *
 * @return
 */
std::string dumpCG() {
  Callgraph* cg = mcgManager.getCallgraph("cg");
  if (cg == nullptr) {
    MCGLogger::logError("No callgraph generated");
    return "";
  }

  std::unique_ptr<MCGWriter> mcgWriter = createWriter(4);
  if (!mcgWriter) {
    MCGLogger::logError("Unable to create a writer");
    return "";
  };

  JsonSink jsonSink;
  mcgWriter->writeActiveGraph(jsonSink);

  return jsonSink.getJson().dump();
}

/**
 * @class CollectCG
 * @brief Plugin action to collect callgraph and dump it as JSON file
 *
 */
class CollectCG : public Fortran::frontend::PluginParseTreeAction {
 public:
  void executeAction() override {
    generateCG(getParsing().parseTree(), getCurrentFile());

    std::string cgString = dumpCG();
    std::unique_ptr<llvm::raw_pwrite_stream> file = ::createOutputFile(getInstance(), getCurrentFile(), "json");
    file->write(cgString.c_str(), cgString.size());
  }
};

/**
 * @class CollectCGwithDot
 * @brief Like CollectCG but also generates a DOT file of callgraph
 *
 */
class CollectCGwithDot : public Fortran::frontend::PluginParseTreeAction {
 public:
  void executeAction() override {
    generateCG(getParsing().parseTree(), getCurrentFile());

    std::string cgString = dumpCG();
    std::unique_ptr<llvm::raw_pwrite_stream> file = ::createOutputFile(getInstance(), getCurrentFile(), "json");
    file->write(cgString.c_str(), cgString.size());

    // dot file
    Callgraph* cg = mcgManager.getCallgraph("cg");
    if (cg == nullptr) {
      MCGLogger::logError("No callgraph generated");
      return;
    }

    dot::DotGenerator dotGen(cg);
    dotGen.generate();

    std::unique_ptr<llvm::raw_pwrite_stream> dotfile = ::createOutputFile(getInstance(), getCurrentFile(), "dot");
    std::string dotString = dotGen.getDotString();
    dotfile->write(dotString.c_str(), dotString.size());
  }
};

/**
 * @class CollectCGNoRename
 * @brief Like CollectCG but does not rename output file
 *
 */
class CollectCGNoRename : public Fortran::frontend::PluginParseTreeAction {
 public:
  void executeAction() override {
    generateCG(getParsing().parseTree(), getCurrentFile());

    std::string cgString = dumpCG();

    std::unique_ptr<llvm::raw_pwrite_stream> file = ::createOutputFile(getInstance(), getCurrentFile(), "");
    file->write(cgString.c_str(), cgString.size());
  }
};

static Fortran::frontend::FrontendPluginRegistry::Add<CollectCG> X("genCG", "Generate Callgraph");
static Fortran::frontend::FrontendPluginRegistry::Add<CollectCGwithDot> Y("genCGwithDot", "Generate Callgraph");
static Fortran::frontend::FrontendPluginRegistry::Add<CollectCGNoRename> Z(
    "genCGNoRename", "Generate Callgraph without renaming output file");

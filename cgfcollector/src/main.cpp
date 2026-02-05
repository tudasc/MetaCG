#include "ParseTreeVisitor.h"

#include <DotIO.h>

class CollectCG : public Fortran::frontend::PluginParseTreeAction {
 public:
  void executeAction() override {
    auto cg = std::make_unique<metacg::Callgraph>();

    ParseTreeVisitor visitor(cg.get(), getCurrentFile().str());
    Fortran::parser::Walk(getParsing().parseTree(), visitor);

    // add edges
    for (auto edge : visitor.getEdges()) {
      auto* callerNode = cg->getOrInsertNode(edge.first);
      auto* calleeNode = cg->getOrInsertNode(edge.second);
      if (!calleeNode || !callerNode) {
        llvm::outs() << "No nodes found for edge: " << edge.first << " -> " << edge.second << "\n";
        continue;
      }

      cg->addEdge(callerNode, calleeNode);
    }

    auto& mcgManager = metacg::graph::MCGManager::get();

    mcgManager.resetManager();
    mcgManager.addToManagedGraphs("test", std::move(cg), true);
    mcgManager.mergeIntoActiveGraph();

    auto mcgWriter = metacg::io::createWriter(3);
    if (!mcgWriter) {
      llvm::errs() << "Unable to create a writer\n";
      return;
    };

    metacg::io::JsonSink jsonSink;
    mcgWriter->writeActiveGraph(jsonSink);

    auto file = createOutputFile("json");
    file->write(jsonSink.getJson().dump().c_str(), jsonSink.getJson().dump().size());
  }
};

class CollectCGwithDot : public CollectCG {
 public:
  void executeAction() override {
    CollectCG::executeAction();

    auto& mcgManager = metacg::graph::MCGManager::get();

    metacg::io::dot::DotGenerator dotGen(mcgManager.getCallgraph("test"));
    dotGen.generate();

    llvm::SmallString<128> outputPath(getInstance().getFrontendOpts().outputFile);
    if (outputPath.empty()) {
      outputPath = getCurrentFile();
    }
    llvm::sys::path::replace_extension(outputPath, "dot");
    std::error_code ec;
    llvm::raw_fd_ostream out(outputPath.str(), ec);
    if (ec) {
      llvm::errs() << "Error opening output file: " << ec.message() << "\n";
      return;
    }
    out << dotGen.getDotString();
  }
};

static Fortran::frontend::FrontendPluginRegistry::Add<CollectCG> X("genCG", "Generate Callgraph");
static Fortran::frontend::FrontendPluginRegistry::Add<CollectCGwithDot> Y("genCGwithDot", "Generate Callgraph");

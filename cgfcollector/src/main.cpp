#include "ParseTreeVisitor.h"

#include <DotIO.h>

class CollectCG : public Fortran::frontend::PluginParseTreeAction {
 public:
  void executeAction() override {
    AL* al = AL::getInstance();

    auto cg = std::make_unique<metacg::Callgraph>();

    // TODO: remove
    if (std::getenv("CUSTOM_DEBUG")) {
      spdlog::set_pattern("%v");
      spdlog::set_level(spdlog::level::debug);
    }

    ParseTreeVisitor visitor(cg.get(), getCurrentFile().str());
    Fortran::parser::Walk(getParsing().parseTree(), visitor);

    // handle potential finalizers from function calls
    for (const auto pf : visitor.getPotentialFinalizers()) {
      auto functions = visitor.getFunctions();
      auto calledIt = std::find_if(functions.begin(), functions.end(),
                                   [&](const auto& f) { return mangleName(*f.symbol) == pf.procedureCalled; });
      if (calledIt == functions.end())
        continue;

      auto arg = calledIt->dummyArgs.begin() + pf.argPos;

      if (!arg->hasBeenInitialized)
        continue;

      for (const auto& edge : pf.finalizerEdges) {
        visitor.getEdges().emplace_back(edge.first, edge.second);
        al->debug("Add edge for potential finalizer: {} -> {}", edge.first, edge.second);
      }
    }

    // add edges
    for (auto edge : visitor.getEdges()) {
      const auto& callerNode = cg->getOrInsertNode(edge.first);
      const auto& calleeNode = cg->getOrInsertNode(edge.second);

      cg->addEdge(callerNode, calleeNode);
    }

    auto& mcgManager = metacg::graph::MCGManager::get();

    mcgManager.resetManager();
    mcgManager.addToManagedGraphs("test", std::move(cg), true);
    mcgManager.mergeIntoActiveGraph(metacg::MergeByName());

    auto mcgWriter = metacg::io::createWriter(4);
    if (!mcgWriter) {
      llvm::errs() << "Unable to create a writer\n";
      return;
    };

    metacg::io::JsonSink jsonSink;
    mcgWriter->writeActiveGraph(jsonSink);

    auto file = createOutputFile("json");
    file->write(jsonSink.getJson().dump().c_str(), jsonSink.getJson().dump().size());

    al->flush();
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

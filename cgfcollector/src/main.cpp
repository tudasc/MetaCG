#include "ParseTreeVisitor.h"

#include <DotIO.h>

class CollectCG : public Fortran::frontend::PluginParseTreeAction {
 public:
  metacg::Callgraph* cg = nullptr;

  ~CollectCG() override {
    // if (cg != nullptr) {
    //   delete cg;
    //   cg = nullptr;
    // }
  }

  void generateCG() {
    if (cg != nullptr) {
      return;
    }

    AL* al = AL::getInstance();

    cg = new metacg::Callgraph();

    // TODO: remove
    if (std::getenv("CUSTOM_DEBUG")) {
      spdlog::set_pattern("%v");
      spdlog::set_level(spdlog::level::debug);
    }

    ParseTreeVisitor visitor(cg, getCurrentFile().str());
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

    al->flush();
  }

  std::string cgToString(metacg::Callgraph* cg) {
    auto& mcgManager = metacg::graph::MCGManager::get();

    mcgManager.resetManager();
    mcgManager.addToManagedGraphs("test", std::unique_ptr<metacg::Callgraph>(cg), true);
    mcgManager.mergeIntoActiveGraph(metacg::MergeByName());

    auto mcgWriter = metacg::io::createWriter(4);
    if (!mcgWriter) {
      llvm::errs() << "Unable to create a writer\n";
      return "";
    };

    metacg::io::JsonSink jsonSink;
    mcgWriter->writeActiveGraph(jsonSink);

    return jsonSink.getJson().dump();
  }

  std::unique_ptr<llvm::raw_pwrite_stream> createOutputFile(llvm::StringRef extension) {
    llvm::SmallString<128> outputPath(getInstance().getFrontendOpts().outputFile);
    if (outputPath.empty()) {
      outputPath = getCurrentFile();
    }
    if (extension != "")
      llvm::sys::path::replace_extension(outputPath, extension);
    std::unique_ptr<llvm::raw_fd_ostream> os;
    std::error_code ec;
    os.reset(new llvm::raw_fd_ostream(outputPath.str(), ec, llvm::sys::fs::OF_TextWithCRLF));
    if (ec) {
      llvm::errs() << "Error opening output file: " << ec.message() << "\n";
      return nullptr;
    }
    return os;
  }

  void executeAction() override {
    generateCG();

    std::string cgString = cgToString(cg);

    auto file = createOutputFile("json");
    file->write(cgString.c_str(), cgString.size());
  }
};

class CollectCGwithDot : public CollectCG {
 public:
  void executeAction() override {
    CollectCG::executeAction();

    metacg::io::dot::DotGenerator dotGen(cg);
    dotGen.generate();

    auto file = CollectCG::createOutputFile("dot");
    std::string dotString = dotGen.getDotString();
    file->write(dotString.c_str(), dotString.size());
  }
};

class CollectCGNoRename : public CollectCG {
 public:
  void executeAction() override {
    generateCG();

    std::string cgString = cgToString(cg);

    auto file = createOutputFile("");
    file->write(cgString.c_str(), cgString.size());
  }
};

static Fortran::frontend::FrontendPluginRegistry::Add<CollectCG> X("genCG", "Generate Callgraph");
static Fortran::frontend::FrontendPluginRegistry::Add<CollectCGwithDot> Y("genCGwithDot", "Generate Callgraph");
static Fortran::frontend::FrontendPluginRegistry::Add<CollectCGNoRename> Z(
    "genCGNoRename", "Generate Callgraph without renaming output file");

#include "headers.h"

class CollectCallersAndCallees : public Fortran::frontend::PluginParseTreeAction {
  class ParseTreeVisitor {
   public:
    ParseTreeVisitor(metacg::Callgraph* cg, std::string currentFileName) : cg(cg), currentFileName(currentFileName) {};

    template <typename T>
    void handleFuncSubStmt(const T& stmt) {
      if (auto* sym = std::get<Fortran::parser::Name>(stmt.t).symbol) {
        functionNames.emplace_back(Fortran::lower::mangle::mangleName(*sym));
        cg->insert(std::make_unique<metacg::CgNode>(functionNames.back(), currentFileName, false, false));
      }
    }
    void handleEndFuncSubStmt() {
      if (!functionNames.empty()) {
        functionNames.pop_back();
      }
    }

    std::vector<std::pair<std::string, std::string>> getEdges() const { return edges; }

    template <typename A>
    bool Pre(const A&) {
      return true;
    }
    template <typename A>
    void Post(const A&) {}

    bool Pre(const Fortran::parser::MainProgram& p) {
      inMainProgram = true;

      if (const auto& maybeStmt = std::get<0>(p.t)) {
        if (!maybeStmt->statement.v.symbol)
          return true;

        functionNames.emplace_back(Fortran::lower::mangle::mangleName(*maybeStmt->statement.v.symbol));
        cg->insert(std::make_unique<metacg::CgNode>(functionNames.back(), currentFileName, false, false));
      }
      return true;
    }

    void Post(const Fortran::parser::MainProgram&) {
      inMainProgram = false;

      if (!functionNames.empty()) {
        functionNames.pop_back();
      }
    }

    bool Pre(const Fortran::parser::FunctionSubprogram&) {
      inFunctionOrSubroutineSubProgram = true;
      return true;
    }
    bool Post(const Fortran::parser::FunctionSubprogram&) {
      inFunctionOrSubroutineSubProgram = false;
      return true;
    }
    bool Pre(const Fortran::parser::SubroutineSubprogram&) {
      inFunctionOrSubroutineSubProgram = true;
      return true;
    }
    bool Post(const Fortran::parser::SubroutineSubprogram&) {
      inFunctionOrSubroutineSubProgram = false;
      return true;
    }

    void Post(const Fortran::parser::ExecutionPart& e) {
      if (!inFunctionOrSubroutineSubProgram && !inMainProgram)
        return;

      auto* node = cg->getNode(functionNames.back());
      if (!node) {
        return;
      }

      node->setHasBody(true);
    }

    void Post(const Fortran::parser::FunctionStmt& f) { handleFuncSubStmt(f); }
    void Post(const Fortran::parser::EndFunctionStmt&) { handleEndFuncSubStmt(); }
    void Post(const Fortran::parser::SubroutineStmt& s) { handleFuncSubStmt(s); }
    void Post(const Fortran::parser::EndSubroutineStmt&) { handleEndFuncSubStmt(); }

    void Post(const Fortran::parser::ProcedureDesignator& p) {
      std::string callee = "";

      // if just the name is called. (as subroutine with call and as function without call)
      if (auto* name = std::get_if<Fortran::parser::Name>(&p.u)) {
        if (!name->symbol)
          return;

        // ignore intrinsic functions
        if (name->symbol->attrs().test(Fortran::semantics::Attr::INTRINSIC))
          return;

        callee = Fortran::lower::mangle::mangleName(*name->symbol);
      }

      // if called from a object with %
      if (auto* compRef = std::get_if<Fortran::parser::ProcComponentRef>(&p.u)) {
        if (!compRef->v.thing.component.symbol)
          return;

        // ignore intrinsic functions TODO check
        // if (compRef->v.thing.component.symbol->attrs().test(Fortran::semantics::Attr::INTRINSIC))
        //   return;

        callee = Fortran::lower::mangle::mangleName(*compRef->v.thing.component.symbol);
      }

      if (functionNames.empty())
        return;

      edges.emplace_back(functionNames.back(), callee);
    }

      }
    }

   private:
    metacg::Callgraph* cg;
    std::vector<std::pair<std::string, std::string>> edges;  // (caller, callee)
    std::string currentFileName;

    std::vector<std::string> functionNames;
    bool inFunctionOrSubroutineSubProgram = false;
    bool inMainProgram = false;
  };

  void executeAction() override {
    auto cg = std::make_unique<metacg::Callgraph>();

    ParseTreeVisitor visitor(cg.get(), getCurrentFile().str());
    Fortran::parser::Walk(getParsing().parseTree(), visitor);

    // add edges
    for (auto edge : visitor.getEdges()) {
      auto* callerNode = cg->getNode(edge.first);
      auto* calleeNode = cg->getNode(edge.second);
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

    // TODO: only debug ?
    metacg::io::dot::DotGenerator dotGen(mcgManager.getCallgraph("test"));
    dotGen.generate();

    llvm::SmallString<128> outputPath(getInstance().getFrontendOpts().outputFile);
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

static Fortran::frontend::FrontendPluginRegistry::Add<CollectCallersAndCallees> X("genCG", "Generate Callgraph");

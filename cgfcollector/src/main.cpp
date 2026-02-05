#include <Callgraph.h>
#include <MCGManager.h>
#include <flang/Frontend/FrontendAction.h>
#include <flang/Frontend/FrontendActions.h>
#include <flang/Frontend/FrontendPluginRegistry.h>
#include <flang/Lower/Mangler.h>
#include <flang/Optimizer/Support/InternalNames.h>
#include <flang/Parser/dump-parse-tree.h>
#include <flang/Parser/parse-tree.h>
#include <flang/Parser/parsing.h>
#include <io/MCGWriter.h>
#include <io/VersionTwoMCGWriter.h>
#include <llvm/Support/raw_ostream.h>
#include <unordered_map>
#include <vector>

class CollectCallersAndCallees : public Fortran::frontend::PluginParseTreeAction {
  struct ParseTreeVisitor {
    std::unordered_map<std::string, std::vector<std::string>> functionCalls;
    std::vector<std::string> functionNames;

    template <typename T>
    void handleFuncSubStmt(const T& stmt) {
      if (auto* sym = std::get<Fortran::parser::Name>(stmt.t).symbol) {
        functionNames.emplace_back(Fortran::lower::mangle::mangleName(*sym));
        functionCalls.emplace(functionNames.back(), std::vector<std::string>());
      }
    }
    void handleEndFuncSubStmt() {
      if (!functionNames.empty()) {
        functionNames.pop_back();
      }
    }

    template <typename A>
    bool Pre(const A&) {
      return true;
    }
    template <typename A>
    void Post(const A&) {}

    bool Pre(const Fortran::parser::MainProgram& p) {
      if (const auto& maybeStmt = std::get<0>(p.t)) {
        if (maybeStmt->statement.v.symbol) {
          functionNames.emplace_back(Fortran::lower::mangle::mangleName(*maybeStmt->statement.v.symbol));
          functionCalls.emplace(functionNames.back(), std::vector<std::string>());
        }
      }
      return true;
    }

    void Post(const Fortran::parser::MainProgram&) {
      if (!functionNames.empty()) {
        functionNames.pop_back();
      }
    }

    void Post(const Fortran::parser::FunctionStmt& f) { handleFuncSubStmt(f); }
    void Post(const Fortran::parser::EndFunctionStmt&) { handleEndFuncSubStmt(); }
    void Post(const Fortran::parser::SubroutineStmt& s) { handleFuncSubStmt(s); }
    void Post(const Fortran::parser::EndSubroutineStmt&) { handleEndFuncSubStmt(); }

    void Post(const Fortran::parser::ProcedureDesignator& p) {
      if (auto* name = std::get_if<Fortran::parser::Name>(&p.u)) {
        if (name->symbol) {
          std::string callee = Fortran::lower::mangle::mangleName(*name->symbol);
          // TODO
          if (name->symbol->attrs().test(Fortran::semantics::Attr::INTRINSIC)) {
            callee = "intrinsic" + callee;
          }
          if (!functionNames.empty()) {
            functionCalls[functionNames.back()].emplace_back(callee);
          }
        }
      }
    }
  };

  void executeAction() override {
    ParseTreeVisitor visitor;
    Fortran::parser::Walk(getParsing().parseTree(), visitor);

    auto cg = std::make_unique<metacg::Callgraph>();
    for (const auto& [functionName, functionCalls] : visitor.functionCalls) {
      auto* node = cg->getOrInsertNode(functionName);
      for (const auto& call : functionCalls) {
        auto* calleeNode = cg->getOrInsertNode(call);
        cg->addEdge(node, calleeNode);
      }
    }

    auto& mcgManager = metacg::graph::MCGManager::get();

    // mcgManager.resetManager();
    mcgManager.addToManagedGraphs("test", std::move(cg), true);
    mcgManager.mergeIntoActiveGraph();

    auto mcgWriter = std::make_unique<metacg::io::VersionTwoMCGWriter>(metacg::getVersionTwoFileInfo({
        std::string("CGCollector"),
        MetaCG_VERSION_MAJOR,
        MetaCG_VERSION_MINOR,
    }));
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

static Fortran::frontend::FrontendPluginRegistry::Add<CollectCallersAndCallees> X("genCG", "Generate Callgraph");

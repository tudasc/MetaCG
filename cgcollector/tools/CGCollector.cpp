#include "JSONManager.h"
#include "MetaCollector.h"

#include "CallGraph.h"

#include <clang/AST/ASTConsumer.h>
#include <clang/Frontend/FrontendAction.h>
#include <clang/Tooling/CommonOptionsParser.h>
#include <clang/Tooling/Tooling.h>

#include <fstream>
#include <iostream>
#include <string>

static llvm::cl::OptionCategory cgc("CGCollector");
static llvm::cl::opt<bool> captureCtorsDtors("capture-ctors-dtors", llvm::cl::desc("Capture calls to Constructors and Destructors"), llvm::cl::cat(cgc));

typedef std::vector<MetaCollector *> MetaCollectorVector;

class CallGraphCollectorConsumer : public clang::ASTConsumer {
 public:
  CallGraphCollectorConsumer(MetaCollectorVector mcs, nlohmann::json &j) : _mcs(mcs), _json(j) {}

  virtual void HandleTranslationUnit(clang::ASTContext &Context) {
    callGraph.setCaptureCtorsDtors(captureCtorsDtors);
    callGraph.TraverseDecl(Context.getTranslationUnitDecl());

    for (const auto mc : _mcs) {
      mc->calculateFor(callGraph);
    }

    convertCallGraphToJSON(callGraph, _json);
  }

 private:
  CallGraph callGraph;
  MetaCollectorVector _mcs;
  nlohmann::json &_json;
};

class CallGraphCollectorFactory : clang::ASTFrontendAction {
 public:
  CallGraphCollectorFactory(MetaCollectorVector mcs, nlohmann::json &j) : _mcs(mcs), _json(j) {}

  std::unique_ptr<clang::ASTConsumer> newASTConsumer() {
    return std::unique_ptr<clang::ASTConsumer>(new CallGraphCollectorConsumer(_mcs, _json));
  }

  std::unique_ptr<clang::ASTConsumer> CreateASTConsumer([[maybe_unused]] clang::CompilerInstance &compiler,
                                                        [[maybe_unused]] llvm::StringRef sr) {
    return std::unique_ptr<clang::ASTConsumer>(new CallGraphCollectorConsumer(_mcs, _json));
  }

 private:
  MetaCollectorVector _mcs;
  nlohmann::json &_json;
};

// TODO handle multiple inputs
int main(int argc, const char **argv) {
  if (argc < 2) {
    return -1;
  }

  clang::tooling::CommonOptionsParser OP(argc, argv, cgc);
  clang::tooling::ClangTool CT(OP.getCompilations(), OP.getSourcePathList());

  nlohmann::json j;
  auto noStmtsCollector = std::make_unique<NumberOfStatementsCollector>();

  MetaCollectorVector mcs{noStmtsCollector.get()};

  CT.run(
      clang::tooling::newFrontendActionFactory<CallGraphCollectorFactory>(new CallGraphCollectorFactory(mcs, j)).get());

  for (const auto mc : mcs) {
    addMetaInformationToJSON(j, mc->getName(), mc->getMetaInformation());
  }

  std::string filename(*OP.getSourcePathList().begin());
  filename = filename.substr(0, filename.find_last_of(".")) + ".ipcg";

  std::ofstream file(filename);
  file << j << std::endl;

  return 0;
}

#include "config.h"

#include "AliasAnalysis.h"
#include "CallGraph.h"
#include "CallgraphToJSON.h"
#include "MetaCollector.h"

#include <clang/AST/ASTConsumer.h>
#include <clang/Frontend/FrontendAction.h>
#include <clang/Tooling/CommonOptionsParser.h>
#include <clang/Tooling/Tooling.h>

#include <fstream>
#include <iostream>
#include <string>

static llvm::cl::OptionCategory cgc("CGCollector");
static llvm::cl::opt<bool> captureCtorsDtors("capture-ctors-dtors",
                                             llvm::cl::desc("Capture calls to Constructors and Destructors"),
                                             llvm::cl::cat(cgc));
static llvm::cl::opt<bool> captureStackCtorsDtors(
    "capture-stack-ctors-dtors",
    llvm::cl::desc(
        "Capture calls to Constructors and Destructors of stack allocated variables. (Only works together with AA)"),
    llvm::cl::cat(cgc));
static llvm::cl::opt<int> metacgFormatVersion("metacg-format-version",
                                              llvm::cl::desc("metacg file version to output, values={1,2}, default=1"),
                                              llvm::cl::cat(cgc));

static llvm::cl::opt<std::string> outputFilenameOption("output", llvm::cl::desc("Output filename to store MetaCG"), llvm::cl::cat(cgc));

/**
 * The classic CG construction and the AA one work a bit differently. The classic one inserts nodes for all function,
 * even if they are never called and do not call any functions themself, the AA one does only include functions that
 * either get called or are calling another function.
 */
static llvm::cl::opt<bool> disableClassicCGConstruction(
    "disable-classic-cgc", llvm::cl::desc("Disable the \"classic\" call graph construction"), llvm::cl::cat(cgc));
static llvm::cl::opt<bool> enableAA("enable-AA", llvm::cl::desc("Enable Alias Analysis (experimental)"),
                                    llvm::cl::cat(cgc));

typedef std::vector<MetaCollector *> MetaCollectorVector;

class CallGraphCollectorConsumer : public clang::ASTConsumer {
 public:
  CallGraphCollectorConsumer(MetaCollectorVector mcs, nlohmann::json &j) : _mcs(mcs), _json(j) {}

  virtual void HandleTranslationUnit(clang::ASTContext &Context) {
    callGraph.setCaptureCtorsDtors(captureCtorsDtors);
    if (!disableClassicCGConstruction) {
      callGraph.TraverseDecl(Context.getTranslationUnitDecl());
    }
    nlohmann::json AliasAnalysisMetadata;
    if (enableAA) {
      calculateAliasInfo(Context.getTranslationUnitDecl(), &callGraph, AliasAnalysisMetadata, metacgFormatVersion,
                         captureCtorsDtors, captureStackCtorsDtors);
    }

    for (const auto mc : _mcs) {
      mc->calculateFor(callGraph);
    }

    convertCallGraphToJSON(callGraph, _json, metacgFormatVersion);
    if (enableAA && metacgFormatVersion >= 2) {
      _json["PointerEquivalenceData"] = AliasAnalysisMetadata;
    }
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
  metacgFormatVersion.setInitialValue(1);  // Have the old file format as default
  outputFilenameOption.setInitialValue("");

  std::cout << "Running metacg::CGCollector (version " << CGCollector_VERSION_MAJOR << '.' << CGCollector_VERSION_MINOR
            << ")\nGit revision: " << MetaCG_GIT_SHA << " LLVM/Clang version: " << LLVM_VERSION_STRING << std::endl;

#if (LLVM_VERSION_MAJOR >= 10) && (LLVM_VERSION_MAJOR <= 12)
  clang::tooling::CommonOptionsParser OP(argc, argv, cgc);
#else
  auto ParseResult = clang::tooling::CommonOptionsParser::create(argc, argv, cgc);
  if (!ParseResult) {
    std::cerr << toString(ParseResult.takeError()) << "\n";
    return -1;
  }
  clang::tooling::CommonOptionsParser &OP = ParseResult.get();
#endif
  clang::tooling::ClangTool CT(OP.getCompilations(), OP.getSourcePathList());

  nlohmann::json j;
  auto noStmtsCollector = std::make_unique<NumberOfStatementsCollector>();
  auto foCollector = std::make_unique<FilePropertyCollector>();
  auto csCollector = std::make_unique<CodeStatisticsCollector>();
  auto mcCollector = std::make_unique<MallocVariableCollector>();
  // auto tyCollector = std::make_unique<UniqueTypeCollector>();
  auto noConditionalBranchesCollector = std::make_unique<NumConditionalBranchCollector>();
  auto noOperationsCollector = std::make_unique<NumOperationsCollector>();
  auto loopDepthCollector = std::make_unique<LoopDepthCollector>();
  auto globalLoopDepthCollector = std::make_unique<GlobalLoopDepthCollector>();

  MetaCollectorVector mcs{noStmtsCollector.get()};
  mcs.push_back(foCollector.get());
  mcs.push_back(csCollector.get());
  mcs.push_back(mcCollector.get());
  // mcs.push_back(tyCollector.get());
  if (metacgFormatVersion > 1) {
    mcs.push_back(noConditionalBranchesCollector.get());
    mcs.push_back(noOperationsCollector.get());
    mcs.push_back(loopDepthCollector.get());
    mcs.push_back(globalLoopDepthCollector.get());
  }

  CT.run(
      clang::tooling::newFrontendActionFactory<CallGraphCollectorFactory>(new CallGraphCollectorFactory(mcs, j)).get());

  for (const auto mc : mcs) {
    addMetaInformationToJSON(j, mc->getName(), mc->getMetaInformation(), metacgFormatVersion);
  }

  for (const auto mc : mcs) {
    mc->addMetaInformationToCompleteJson(j, metacgFormatVersion);
  }

  // Default to sourcefile.ipcg
  std::string filename(*OP.getSourcePathList().begin());
  if (outputFilenameOption.empty()) {
    filename = filename.substr(0, filename.find_last_of('.')) + ".ipcg";
  } else {
    filename = outputFilenameOption;
  }

  std::ofstream file(filename);
  file << j << std::endl;

  return 0;
}

#include "collector/CodeStatisticsCollector.h"
#include "collector/GlobalLoopDepthCollector.h"
#include "collector/LoopDepthCollector.h"
#include "collector/MallocVariableCollector.h"
#include "collector/NumConditionalBranchCollector.h"
#include "collector/NumOperationsCollector.h"
#include "collector/NumStatementsCollector.h"
#include "collector/OverrideCollector.h"
#include "collector/UniqueTypeCollector.h"

#include <clang/Tooling/CommonOptionsParser.h>
#include <clang/Tooling/Tooling.h>

#include "CallGraphCollectionAction.h"
#include "nlohmann/json.hpp"

#include "Plugin.h"
#include "SharedDefs.h"

#include "metadata/BuiltinMD.h"
#include <iostream>

#include "spdlog/spdlog.h"

using namespace llvm::cl;

static OptionCategory cgc("CGCollector");

static opt<bool> captureCtorsDtors("capture-ctors-dtors",
                                   desc("Capture calls to Constructors and Destructors <default=false>"), init(false),
                                   cat(cgc));
static opt<bool> captureStackCtorsDtors(
    "capture-stack-ctors-dtors",
    desc("This is here for compatibility reasons and is ignored, it was originally used to:"
         "\nCapture calls to Constructors and Destructors of stack allocated variables."),
    init(false), Hidden, cat(cgc));

static opt<bool> captureNewDeleteCalls(
    "capture-new-delete-calls", desc("Capture calls to the new/new[] and delete/delete[] functions <default=false>"),
    init(false), cat(cgc));

static opt<bool> captureImplicits("capture-implicits",
                                  desc("Capture calls to functions that are only implicit like builtins and "
                                       "unspecified class-member-functions <default=false>"),
                                  init(false), cat(cgc));

static opt<bool> inferCtorsDtors("infer-ctors-dtors",
                                  desc("Infer calls to constructors and destructurs through inheritance chains and  "
                                       "infer calls to destructors based on scopes / lifetimes <default=false>"),
                                  init(false), cat(cgc));

enum class Collectors {
  None,
  NumStatements,
  CodeStatistics,
  LoopDepth,
  GlobalLoopDepth,
  MallocVariable,
  NumConditionalBranches,
  NumOperations,
  UniqueTypes,
  OverrideMD,
  All
};

static bits<Collectors> collectorBits(
    desc("Builtin collections:"),
    values(clEnumValN(Collectors::None, "None", "don't use collectors"),
           clEnumValN(Collectors::NumStatements, "NumStatements", "number of statements"),
           clEnumValN(Collectors::CodeStatistics, "CodeStatistics", "number of declared variables"),
           clEnumValN(Collectors::LoopDepth, "LoopDepth", "nesting level of loops"),
           clEnumValN(Collectors::GlobalLoopDepth, "GlobalLoopDepth", "global nesting level of loops "),
           clEnumValN(Collectors::MallocVariable, "MallocVariable", "number of mallocs"),
           clEnumValN(Collectors::NumConditionalBranches, "NumConditionalBranches", "number of branches"),
           clEnumValN(Collectors::NumOperations, "NumOperations", "number of operations"),
           clEnumValN(Collectors::UniqueTypes, "UniqueTypes", "number of unique types"),
           clEnumValN(Collectors::OverrideMD, "OverrideMD", "overriding and overridden functions"),
           clEnumValN(Collectors::All, "All", "use all collectors")),
    cat(cgc));

static opt<int> metacgFormatVersion("metacg-format-version",
                                    desc("metacg file version to output, values={1,2}, default=2"), init(2), cat(cgc));
/**
 * The classic CG construction and the AA one work a bit differently. The classic one inserts nodes for all function,
 * even if they are never called and do not call any functions themself, the AA one does only include functions that
 * either get called or are calling another function.
 */
static opt<bool> unused1("disable-classic-cgc", cat(cgc),
                         desc("This is only here to allow for direct substitution of the old cgcollector in scripts"),
                         Hidden);

static opt<bool> unused2("output", cat(cgc),
                         desc("This is only here to allow for direct substitution of the old cgcollector in scripts"),
                         Hidden);

static opt<AliasAnalysisLevel> aliasAssumption(
    "alias-model", desc("How to handle function pointers:"),
    values(clEnumVal(No, "Treat function pointers as own function, do not analyse"),
           clEnumVal(
               All,
               "Treat all available valid function signatures for a given function pointer as potential call target ")),
    cat(cgc), init(All));

static opt<bool> standalone("whole-program", desc("Treat all undefined functions as calls to shared libraries"),
                            cat(cgc), init(false));

static opt<bool> prune("prune", desc("Remove uncalled and undefined functions"), cat(cgc), init(false));

static list<std::string> pluginPaths("pluginPaths", desc("option list"), cat(cgc), CommaSeparated);
enum class LogLevel { Trace, Debug, Info, Warning, Error, /* Critical,*/ Off };
static opt<LogLevel> LoggingLevel("log-level", desc("Select log level"),
                                  values(clEnumValN(LogLevel::Trace, "trace", "Enable Trace and higher logs"),
                                         clEnumValN(LogLevel::Debug, "debug", "Enable Debug and higher logs"),
                                         clEnumValN(LogLevel::Info, "info", "Enable Info and higher logs"),
                                         clEnumValN(LogLevel::Warning, "warning", "Enable Warning and higher logs"),
                                         clEnumValN(LogLevel::Error, "error", "Enable only Error logs"),
                                         clEnumValN(LogLevel::Off, "off", "Disable all logging")),
                                  init(LogLevel::Info), cat(cgc));

int main(int argc, const char** argv) {
#if (LLVM_VERSION_MAJOR >= 10) && (LLVM_VERSION_MAJOR <= 12)
  clang::tooling::CommonOptionsParser OP(argc, argv, cgc);
#else
  auto ParseResult = clang::tooling::CommonOptionsParser::create(argc, argv, cgc);
  if (!ParseResult) {
    std::cerr << toString(ParseResult.takeError());
    return -1;
  }
  clang::tooling::CommonOptionsParser& OP = ParseResult.get();
#endif

  switch (LoggingLevel) {
    case LogLevel::Trace:
      spdlog::set_level(spdlog::level::trace);
      break;
    case LogLevel::Debug:
      spdlog::set_level(spdlog::level::debug);
      break;
    case LogLevel::Info:
      spdlog::set_level(spdlog::level::info);
      break;
    case LogLevel::Warning:
      spdlog::set_level(spdlog::level::warn);
      break;
    case LogLevel::Error:
      spdlog::set_level(spdlog::level::err);
      break;
    case LogLevel::Off:
      spdlog::set_level(spdlog::level::off);
      break;
  }
  spdlog::set_pattern("[%Y-%m-%d %H:%M:%S.%e] [errconsole] [%l] %v");

  clang::tooling::ClangTool CT(OP.getCompilations(), OP.getSourcePathList());
  std::vector<Plugin*> mcs = {};
  mcs.reserve(9 /*number of builtin collectors*/ + pluginPaths.size());

  if (collectorBits.getBits() == 0 || collectorBits.isSet(Collectors::None)) {
    SPDLOG_INFO("No collector-suite specified, disabling all collectors");
  } else if (collectorBits.isSet(Collectors::All)) {
    SPDLOG_INFO("Enabling all built in collectors");
    mcs = {new NumberOfStatementsCollector(), new CodeStatisticsCollector(),       new MallocVariableCollector(),
           new UniqueTypeCollector(),         new NumConditionalBranchCollector(), new NumOperationsCollector(),
           new LoopDepthCollector(),          new GlobalLoopDepthCollector(),      new OverrideCollector()};
  } else {
    // Builtin Metadata Collection
    if (collectorBits.isSet(Collectors::NumStatements)) {
      mcs.push_back(new NumberOfStatementsCollector());
    }

    if (collectorBits.isSet(Collectors::CodeStatistics)) {
      mcs.push_back(new CodeStatisticsCollector());
    }

    if (collectorBits.isSet(Collectors::MallocVariable)) {
      mcs.push_back(new MallocVariableCollector());
    }

    if (collectorBits.isSet(Collectors::UniqueTypes)) {
      mcs.push_back(new UniqueTypeCollector());
    }

    if (collectorBits.isSet(Collectors::NumConditionalBranches)) {
      mcs.push_back(new NumConditionalBranchCollector());
    }

    if (collectorBits.isSet(Collectors::NumOperations)) {
      mcs.push_back(new NumOperationsCollector());
    }

    if (collectorBits.isSet(Collectors::LoopDepth)) {
      mcs.push_back(new LoopDepthCollector());
    }

    if (collectorBits.isSet(Collectors::GlobalLoopDepth)) {
      mcs.push_back(new GlobalLoopDepthCollector());
    }

    if (collectorBits.isSet(Collectors::OverrideMD)) {
      mcs.push_back(new OverrideCollector());
    }
  }

  // Plugin Metadata Collection
  for (const auto& pluginPath : pluginPaths) {
    SPDLOG_INFO("Loading external collector from: {}", pluginPath);
    if (Plugin* p = loadPlugin(pluginPath); p) {
      mcs.push_back(p);
    }
  }

  std::unique_ptr<CallGraphCollectorAction> const cgca2 =
      std::make_unique<CallGraphCollectorAction>(mcs, metacgFormatVersion, captureCtorsDtors, captureNewDeleteCalls,
                                                 captureImplicits, inferCtorsDtors, prune, standalone, aliasAssumption);

  CT.run(clang::tooling::newFrontendActionFactory<CallGraphCollectorAction>(cgca2.get()).get());

  for (auto& entry : mcs) {
    delete entry;
  }

  return 0;
}

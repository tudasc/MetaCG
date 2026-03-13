/**
 * File: Plugin.cpp
 * License: Part of the MetaCG project. Licensed under BSD 3 clause license. See LICENSE.txt file at
 * https://github.com/tudasc/metacg/LICENSE.txt
 */
#include "cage/CaGe.h"
#include "cage/interface/CaGePlugin.h"

#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"
#include "llvm/Support/DynamicLibrary.h"

#include "cage/generator/CallgraphGenerator.h"
#include "cage/generator/FileExporter.h"

using namespace llvm;

using namespace llvm::cl;

static OptionCategory cageOpts("CaGe");

static opt<bool> cageVerbose("cage-verbose", desc("Print debugging output"), cat(cageOpts), init(false));

static opt<cage::PTAType> pta(
    "pta", desc("Points-to analysis of indirect calls:"),
    values(clEnumValN(cage::PTAType::No, "no", "Ignore indirect calls"),
           clEnumValN(
               cage::PTAType::BySignature, "signature",
               "Treat all available valid function signatures for a given function pointer as potential call target ")),
    cat(cageOpts), init(cage::PTAType::No));

static opt<std::string> cgout("cg-file", desc("Output file for the generated call graph"), cat(cageOpts), init(""));

static list<std::string> pluginPaths("plugin-paths", desc("option list"), cat(cageOpts), CommaSeparated);

namespace cage {

Plugin* loadPlugin(const std::string& pluginPath) {
  metacg::MCGLogger::instance().getConsole()->debug("Loading plugin");
  std::string err;
  auto lib = sys::DynamicLibrary::getPermanentLibrary(pluginPath.c_str(), &err);
  if (!lib.isValid()) {
    metacg::MCGLogger::instance().getErrConsole()->error("cannot locate the library at {}!", pluginPath);
    metacg::MCGLogger::instance().getErrConsole()->error("Reason: {}", err);
    return nullptr;
  }
  metacg::MCGLogger::instance().getConsole()->trace("Getting collection object from plugin {}", pluginPath);
  void* sym = lib.getAddressOfSymbol("getPlugin");
  if (!sym) {
    metacg::MCGLogger::instance().getErrConsole()->error(
        "Could not load collectors from plugin, no Function \"getPlugin()\"!");
    return nullptr;
  }
  auto getPlugin = reinterpret_cast<Plugin* (*)()>(sym);
  Plugin* loadedPlugin = getPlugin();
  metacg::MCGLogger::logInfo("Successfully loaded Plugin: {}", loadedPlugin->getPluginName());
  return loadedPlugin;
}

PreservedAnalyses CaGe::run(Module& M, ModuleAnalysisManager& MA) {
  if (cageVerbose) {
    outs() << "Running CaGe in verbose mode\n";
  }

  // First check explicit option
  std::string outfile = cgout.getValue();
  if (outfile.empty()) {
    // If empty, check environment variable
    if (const auto* cgNameEnv = std::getenv("CAGE_CG")) {
      outfile = cgNameEnv;
    } else {
      // Default output file
      outfile = "cage_callgraph.mcg";
    }
  }

  Generator gen(pta);
  gen.addPlugin(std::make_unique<FileExporter>(outfile));

  // Load external plugins
  for (const auto& pluginPath : pluginPaths) {
    SPDLOG_INFO("Loading external plugin from: {}", pluginPath);
    if (auto p = std::unique_ptr<Plugin>(loadPlugin(pluginPath))) {
      gen.addPlugin(std::move(p));
    }
  }

  if (!gen.run(M, &MA))
    return PreservedAnalyses::all();

  return PreservedAnalyses::none();
}
}  // namespace cage

llvm::PassPluginLibraryInfo getPluginInfo() {
  return {
    LLVM_PLUGIN_API_VERSION, "CaGe", "0.2", [](PassBuilder& PB) {
    // allow registration via optlevel (non-lto)
#if LLVM_VERSION_MAJOR >= 20
      PB.registerOptimizerLastEPCallback([](ModulePassManager& PM, OptimizationLevel, ThinOrFullLTOPhase) {
#else
      PB.registerOptimizerLastEPCallback([](ModulePassManager& PM, OptimizationLevel) {
#endif
        outs() << "Registering CaGe to run during opt\n";
        PM.addPass(cage::CaGe());
      });

      // registering via optlevel during lto appears to still be broken
      PB.registerFullLinkTimeOptimizationLastEPCallback([](ModulePassManager& PM, OptimizationLevel o) {
        outs() << "Registering CaGe to run during full LTO\n";
        PM.addPass(cage::CaGe());
      });

      // allow registration via pipeline parser
      PB.registerPipelineParsingCallback(
          [](StringRef Name, ModulePassManager& MPM, ArrayRef<llvm::PassBuilder::PipelineElement>) {
            if (Name == "CaGe") {
              outs() << "Registering CaGe to run as pipeline described\n";
              MPM.addPass(cage::CaGe());

              return true;
            } else {
              outs() << "Did not register CaGe\n";
            }
            return false;
          });
    }
  };
}

extern "C" LLVM_ATTRIBUTE_WEAK ::llvm::PassPluginLibraryInfo llvmGetPassPluginInfo() { return getPluginInfo(); }
/**
 * File: PGISMain.cpp
 * License: Part of the MetaCG project. Licensed under BSD 3 clause license. See LICENSE.txt file at
 * https://github.com/tudasc/metacg/LICENSE.txt
 */

#include "config/GlobalConfig.h"
#include "config/ParameterConfig.h"

#include "CubeReader.h"
#include "DotIO.h"
#include "ErrorCodes.h"
#include "ExtrapEstimatorPhase.h"
#include "IPCGEstimatorPhase.h"
#include "LegacyMCGReader.h"
#include "LoggerUtil.h"
#include "MetaData/PGISMetaData.h"
#include "PiraMCGProcessor.h"
#include "Utility.h"
#include "io/VersionTwoMCGReader.h"
#include "io/VersionTwoMCGWriter.h"
#include <loadImbalance/LIEstimatorPhase.h>
#include <loadImbalance/LIMetaData.h>
#include <loadImbalance/OnlyMainEstimatorPhase.h>

#include "cxxopts.hpp"

#include <filesystem>

namespace metacg::pgis::options {
template <typename T>
auto optType(T& obj) {
  return cxxopts::value<arg_t<decltype(obj)>>();
}
}  // namespace metacg::pgis::options

using namespace metacg;
using namespace pira;
using namespace metacg::pgis::options;

static pgis::ErrorCode readFromCubeFile(const std::filesystem::path& cubeFilePath, Config* cfgPtr) {
  if (!std::filesystem::exists(cubeFilePath)) {
    metacg::MCGLogger::instance().getConsole()->error("Cube file does not exist. (" + cubeFilePath.string() + ")");
    return pgis::FileDoesNotExist;
  }

  if (const auto ext = cubeFilePath.extension(); ext.string() != ".cubex") {
    metacg::MCGLogger::instance().getConsole()->error("Unknown file format. (" + cubeFilePath.string() + ")");
    return pgis::UnknownFileFormat;
  }

  auto& mcgm = graph::MCGManager::get();
  // TODO: Change to std::filesystem
  pgis::buildFromCube(cubeFilePath, cfgPtr, mcgm);
  return pgis::SUCCESS;
}

void registerEstimatorPhases(metacg::pgis::PiraMCGProcessor& consumer, Config* c, bool isIPCG, float runtimeThreshold,
                             bool fillInstrumentationGaps) {
  // XXX: Does this somehow add vital functionality that always has to run, or so?
  auto statEstimator = new StatisticsEstimatorPhase(false, consumer.getCallgraph());
  consumer.registerEstimatorPhase(statEstimator);

  // Actually do the selection
  if (!isIPCG) {
    metacg::MCGLogger::instance().getConsole()->info("New runtime threshold for profiling: ${}$", runtimeThreshold);
    consumer.registerEstimatorPhase(new RuntimeEstimatorPhase(consumer.getCallgraph(), runtimeThreshold));
  } else {
    HeuristicSelection::HeuristicSelectionEnum heuristicMode = pgis::config::getSelectedHeuristic();
    switch (heuristicMode) {
      case HeuristicSelection::HeuristicSelectionEnum::STATEMENTS: {
        const int nStmt = 2000;
        metacg::MCGLogger::instance().getConsole()->info("New runtime threshold for profiling: ${}$", nStmt);
        consumer.registerEstimatorPhase(
            new StatementCountEstimatorPhase(nStmt, consumer.getCallgraph(), true, statEstimator));
      } break;
      case HeuristicSelection::HeuristicSelectionEnum::CONDITIONALBRANCHES:
        consumer.registerEstimatorPhase(
            new ConditionalBranchesEstimatorPhase(0, consumer.getCallgraph(), statEstimator));
        break;
      case HeuristicSelection::HeuristicSelectionEnum::CONDITIONALBRANCHES_REVERSE:
        consumer.registerEstimatorPhase(
            new ConditionalBranchesReverseEstimatorPhase(0, consumer.getCallgraph(), statEstimator));
        break;
      case HeuristicSelection::HeuristicSelectionEnum::FP_MEM_OPS:
        consumer.registerEstimatorPhase(new FPAndMemOpsEstimatorPhase(0, consumer.getCallgraph(), statEstimator));
        break;
      case HeuristicSelection::HeuristicSelectionEnum::LOOPDEPTH:
        consumer.registerEstimatorPhase(new LoopDepthEstimatorPhase(0, consumer.getCallgraph(), statEstimator));
        break;
      case HeuristicSelection::HeuristicSelectionEnum::GlOBAL_LOOPDEPTH:
        consumer.registerEstimatorPhase(new GlobalLoopDepthEstimatorPhase(0, consumer.getCallgraph(), statEstimator));
        break;
    }
  }
  if (fillInstrumentationGaps) {
    consumer.registerEstimatorPhase(new FillInstrumentationGapsPhase(consumer.getCallgraph()));
  }
  consumer.registerEstimatorPhase(new StatisticsEstimatorPhase(true, consumer.getCallgraph()));
  consumer.registerEstimatorPhase(new StoreInstrumentationDecisionsPhase(consumer.getCallgraph()));
}

/**
 * Checks command line option for value, adds it to global config and returns the set value for local use.
 */
template <typename OptTy, typename ResTy>
typename OptTy::type storeOpt(const OptTy& opt, const ResTy& res) {
  using Ty = typename OptTy::type;
  Ty tmp = res[opt.cliName].template as<Ty>();
  pgis::config::GlobalConfig::get().putOption(opt.cliName, tmp);
  return tmp;
}

/**
 * Template only to use type deduction
 */
template <typename Opt>
void setDebugLevel(const Opt& result) {
  const int printDebug = storeOpt(debugLevel, result);
  if (printDebug == 1) {
    spdlog::set_level(spdlog::level::debug);
  } else if (printDebug == 2) {
    spdlog::set_level(spdlog::level::trace);
  } else {
    spdlog::set_level(spdlog::level::info);
  }
}

int main(int argc, char** argv) {
  auto console = metacg::MCGLogger::instance().getConsole();
  auto errconsole = metacg::MCGLogger::instance().getErrConsole();

  if (argc == 1) {
    errconsole->error("Too few arguments. Use --help to show help.");
    exit(pgis::TooFewProgramArguments);
  }

  auto& gConfig = pgis::config::GlobalConfig::get();
  cxxopts::Options opts("PGIS", "Generating low-overhead instrumentation selections.");

  /* The marked options should go away for the PGIS release */
  // clang-format off
  opts.add_options()
    ("m,mangled", "Use mangled names", cxxopts::value<bool>()->default_value("false"))
    // not sure
    (outBaseDir.cliName, "Base path to place instrumentation configuration", optType(outBaseDir)->default_value("out"))
    // from here: keep all
    (staticSelection.cliName, "Apply static selection", optType(staticSelection)->default_value("false"))
    (cubeFilePath.cliName, "Cube file for dynamic instrumentation", optType(cubeFilePath)->default_value(""))
    ("h,help", "Show help", cxxopts::value<bool>()->default_value("false"))
    (extrapConfig.cliName, "File to read Extra-P info from", optType(extrapConfig)->default_value(""))
    (modelFilter.cliName, "Use Extra-P models to filter only.", optType(modelFilter)->default_value("false"))
    (runtimeOnly.cliName, "Do not use model, but multiple runtimes", optType(runtimeOnly)->default_value("false"))
    ("w, whitelist", "Filter nodes through given whitelist", cxxopts::value<std::string>()->default_value(""))
    (debugLevel.cliName, "Whether debug messages should be printed", optType(debugLevel)->default_value("0"))
    (scorepOut.cliName, "Write instrumentation file with Score-P syntax", optType(scorepOut)->default_value("false"))
    (ipcgExport.cliName, "Export the profiling info into IPCG file", optType(ipcgExport)->default_value("false"))
    (parameterFileConfig.cliName, "File path to configuration file containing analysis parameters", optType(parameterFileConfig)->default_value(""))
    (lideEnabled.cliName, "Enable load imbalance detection (PIRA LIDe)", optType(lideEnabled)->default_value("false"))
    (metacgFormat.cliName, "Selects the MetaCG format to expect", optType(metacgFormat)->default_value("1"))
    (targetOverhead.cliName, "The target overhead in percent * 10 (>1000)", optType(targetOverhead)->default_value(targetOverhead.defaultValue))
    (prevOverhead.cliName, "The previous overhead in percent * 10 (>1000)", optType(prevOverhead)->default_value(prevOverhead.defaultValue))
    (dotExport.cliName, "Export call-graph as dot-file after every phase.", optType(dotExport)->default_value(dotExport.defaultValue))
    (cubeShowOnly.cliName, "Print inclusive time for main", optType(cubeShowOnly)->default_value("false"))
    (useCallSiteInstrumentation.cliName, "Enable experimental call-site instrumentation", optType(useCallSiteInstrumentation)->default_value("false"))
    (onlyInstrumentEligibleNodes.cliName, "Only select nodes for instrumentation that can be instrumented", optType(onlyInstrumentEligibleNodes)->default_value(onlyInstrumentEligibleNodes.defaultValue))
    (heuristicSelection.cliName, "Select the heuristic to use for node selection", optType(heuristicSelection)->default_value(heuristicSelection.defaultValue))
    (cuttoffSelection.cliName, "Select the algorithm to determine the cutoff for node selection", optType(cuttoffSelection)->default_value(cuttoffSelection.defaultValue))
    (fillGaps.cliName, "Fills gaps in the cg of instrumented functions", optType(fillGaps)->default_value(fillGaps.defaultValue))
    (overheadSelection.cliName, "Algorithm to deal with to high overheads", optType(overheadSelection)->default_value(overheadSelection.defaultValue))
    (sortDotEdges.cliName, "Sort edges in DOT graph lexicographically", optType(sortDotEdges)->default_value(sortDotEdges.defaultValue));
  // clang-format on

  Config c;

  auto result = opts.parse(argc, argv);

  if (result.count("help")) {
    std::cout << opts.help() << "\n";
    return metacg::pgis::SUCCESS;
  }

  /* Initialize the debug level */
  setDebugLevel(result);

  /* Enable DOT export */
  const DotExportSelection enableDotExport;
  storeOpt(dotExport, result);
  storeOpt(sortDotEdges, result);

  /* Enable static, aggregated statement filtering */
  const bool applyStaticFilter = storeOpt(staticSelection, result);

  /* Store the provided cube file path, if any */
  storeOpt(cubeFilePath, result);

  /* Use Extra-P performance-model-based filtering or runtime-based only */
  const bool applyModelFilter = storeOpt(modelFilter, result);
  const bool extrapRuntimeOnly = storeOpt(runtimeOnly, result);
  storeOpt(extrapConfig, result);

  /* Whether Score-P output file format should be used */
  const bool useScorepFormat = storeOpt(scorepOut, result);
  if (useScorepFormat) {
    console->info("Setting Score-P Output Format");
  }

  /* Enable MPI Load-Imbalance detection */
  const bool enableLide = storeOpt(lideEnabled, result);
  storeOpt(parameterFileConfig, result);

  /* Whether gaps in the instrumentation should be filled by a separate pass */
  const bool fillInstrumentationGaps = storeOpt(fillGaps, result);

  /* Which MetaCG file format version to use/expect */
  int mcgVersion = storeOpt(metacgFormat, result);

  /* Enable call-site instrumentation (TODO: Is the instrumentor functional?) */
  storeOpt(useCallSiteInstrumentation, result);

  /* Only select nodes for instrumentation that can actually be instrumented */
  storeOpt(onlyInstrumentEligibleNodes, result);

  /* Select algorithms to use in the static selection */
  storeOpt(heuristicSelection, result);
  storeOpt(cuttoffSelection, result);

  /* Select targeted overhead */
  const int targetOverheadArg = storeOpt(targetOverhead, result);
  const int prevOverheadArg = storeOpt(prevOverhead, result);
  if ((prevOverheadArg != 0 || targetOverheadArg != 0) && targetOverheadArg < 1000) {
    errconsole->error("Target overhead in percent must greater than 100%");
    exit(pgis::ErroneousOverheadConfiguration);
  }
  OverheadSelection overheadMode = storeOpt(overheadSelection, result);
  if (targetOverheadArg != 0 && overheadMode.mode == OverheadSelection::OverheadSelectionEnum::None) {
    errconsole->warn("Overhead mode is none but target overhead is specified");
  }

  if (mcgVersion < 2 &&
      pgis::config::getSelectedHeuristic() != HeuristicSelection::HeuristicSelectionEnum::STATEMENTS) {
    errconsole->error("Heuristics other than 'statements' are not supported with metacg format 1");
    exit(pgis::ErroneousHeuristicsConfiguration);
  }

  /* Where should the instrumentation configuration be written to */
  c.outputFile = storeOpt(outBaseDir, result);

  /*
   * Optional arguments are paresed / consumed, the positional arguments are next.
   * They can be accessed just via argv[]
   */

  // Use the filesystem STL functions for all of the path bits
  const std::filesystem::path metacgFile(argv[argc - 1]);
  if (!std::filesystem::exists(metacgFile)) {
    errconsole->error("The file {} does not exist", metacgFile.string());
    exit(pgis::FileDoesNotExist);
  }
  const auto mcgExtension = metacgFile.extension();
  c.appName = metacgFile.stem().string();

  console->info("MetaCG file: {}", metacgFile.string());
  console->info("Using AppName: {}", c.appName);

  const float runTimeThreshold = .0f;
  auto& consumer = metacg::pgis::PiraMCGProcessor::get();
  auto& mcgm = metacg::graph::MCGManager::get();
  mcgm.addToManagedGraphs("emptyGraph", std::make_unique<metacg::Callgraph>());
  consumer.setConfig(&c);

  if (!gConfig.getVal(extrapConfig).empty()) {
    const std::filesystem::path extrapConfigFile(gConfig.getVal(extrapConfig));
    if (!std::filesystem::exists(extrapConfigFile)) {
      errconsole->error("Extra-P configuration file does not exist: {}", extrapConfigFile.string());
      exit(metacg::pgis::FileDoesNotExist);
    }
    console->info("Reading Extra-P configuration from {}", extrapConfigFile.string());
    consumer.setExtrapConfig(extrapconnection::getExtrapConfigFromJSON(extrapConfigFile));
  }

  /* To briefly inspect a CUBE profile for inclusive runtime of main */
  if (storeOpt(cubeShowOnly, result)) {
    const std::filesystem::path cubeFile(gConfig.getVal(cubeFilePath));
    if (auto ret = readFromCubeFile(cubeFile, &c); ret != metacg::pgis::SUCCESS) {
      exit(ret);
    }

    consumer.printMainRuntime();
    return metacg::pgis::SUCCESS;
  }

  if (metacgFile.extension() == ".ipcg" || metacgFile.extension() == ".mcg") {
    metacg::io::FileSource fs(metacgFile);
    if (mcgVersion == 1) {
      metacg::pgis::io::VersionOneMetaCGReader mcgReader(fs);
      metacg::graph::MCGManager::get().addToManagedGraphs("newGraph", std::make_unique<Callgraph>());
      mcgReader.read();
    } else if (mcgVersion == 2) {
      metacg::io::VersionTwoMetaCGReader mcgReader(fs);
      metacg::graph::MCGManager::get().addToManagedGraphs("newGraph", mcgReader.read());
      // XXX Find better way to do this: both conceptually and complexity-wise
      pgis::attachMetaDataToGraph<pira::PiraOneData>(mcgm.getCallgraph());
      pgis::attachMetaDataToGraph<pira::BaseProfileData>(mcgm.getCallgraph());
      pgis::attachMetaDataToGraph<LoadImbalance::LIMetaData>(mcgm.getCallgraph());
      pgis::attachMetaDataToGraph<pira::PiraTwoData>(mcgm.getCallgraph());
    }

    console->info("Read MetaCG with {} nodes.", mcgm.getCallgraph()->size());
    consumer.setCG(mcgm.getCallgraph());

    if (applyStaticFilter) {
      // load imbalance detection
      // ========================
      if (enableLide) {
        console->info("Using trivial static analysis for load imbalance detection (OnlyMainEstimatorPhase");
        // static instrumentation -> OnlyMainEstimatorPhase
        if (!result.count("cube")) {
          consumer.registerEstimatorPhase(new LoadImbalance::OnlyMainEstimatorPhase(consumer.getCallgraph()));
          consumer.applyRegisteredPhases();

          return metacg::pgis::SUCCESS;
        }
      } else {
        registerEstimatorPhases(consumer, &c, true, 0, fillInstrumentationGaps);
        consumer.applyRegisteredPhases();
        consumer.removeAllEstimatorPhases();
      }
    }
  }
  consumer.registerEstimatorPhase(new AttachInstrumentationResultsEstimatorPhase(consumer.getCallgraph()));
  if (result.count("cube")) {
    const std::filesystem::path cubeFile(gConfig.getVal(cubeFilePath));
    if (auto ret = readFromCubeFile(cubeFile, &c); ret != metacg::pgis::SUCCESS) {
      exit(ret);
    }

    // load imbalance detection
    // ========================
    if (enableLide) {
      console->info("Using load imbalance detection mode");
      auto& pConfig = pgis::config::ParameterConfig::get();

      if (!pConfig.getLIConfig()) {
        errconsole->error(
            "Provide configuration for load imbalance detection. Refer to PIRA's README for further details.");
        return pgis::ErroneousHeuristicsConfiguration;
      }

      // attention: moves out liConfig!
      consumer.registerEstimatorPhase(
          new LoadImbalance::LIEstimatorPhase(std::move(pConfig.getLIConfig()), consumer.getCallgraph()));

      consumer.applyRegisteredPhases();

      // should be set for working load imbalance detection
      if (result.count("export")) {
        console->info("Exporting load imbalance data to IPCG file {}.", metacgFile.string());
        console->warn(
            "The old annotate mechanism has been removed and this functionality has not been tested with MCGWriter.");

        metacg::io::VersionTwoMCGWriter mcgWriter{};
        metacg::io::JsonSink jsonSink;
        mcgWriter.write(jsonSink);
        {
          std::ofstream out(metacgFile);
          out << jsonSink.getJson().dump(4) << std::endl;
        }

      } else {
        console->warn("--export flag is highly recommended for load imbalance detection");
      }

      return metacg::pgis::SUCCESS;
    } else {
      c.totalRuntime = c.actualRuntime;
      /* This runtime threshold currently unused */
      registerEstimatorPhases(consumer, &c, false, runTimeThreshold, fillInstrumentationGaps);
      console->info("Registered estimator phases");
    }
  }

  if (result["extrap"].count()) {
    // test whether PIRA II configPtr is present
    auto& pConfig = pgis::config::ParameterConfig::get();
    if (!pConfig.getPiraIIConfig()) {
      console->error("Provide PIRA II configuration in order to use Extra-P estimators.");
      return pgis::ErroneousHeuristicsConfiguration;
    }

    consumer.attachExtrapModels();

    if (applyModelFilter) {
      console->info("Applying model filter");
      consumer.registerEstimatorPhase(
          new pira::ExtrapLocalEstimatorPhaseSingleValueFilter(consumer.getCallgraph(), true, extrapRuntimeOnly));
    } else {
      console->info("Applying model expander");
      consumer.registerEstimatorPhase(
          new pira::ExtrapLocalEstimatorPhaseSingleValueExpander(consumer.getCallgraph(), true, extrapRuntimeOnly));
    }
    if (fillInstrumentationGaps) {
      consumer.registerEstimatorPhase(new FillInstrumentationGapsPhase(consumer.getCallgraph()));
    }
    // XXX Should this be done after filter / expander were run? Currently we do this after model creation, yet,
    // *before* running the estimator phase
    if (result.count("export")) {
      console->info("Exporting to IPCG file.");
      console->warn(
          "The old annotate mechanism has been removed and this functionality has not been tested with MCGWriter.");

      metacg::io::VersionTwoMCGWriter mcgWriter;
      metacg::io::JsonSink jsonSink;
      mcgWriter.write(jsonSink);
      {
        std::ofstream out(metacgFile);
        out << jsonSink.getJson().dump(4) << std::endl;
      }
    }
  }

  if (enableDotExport.mode == DotExportSelection::DotExportEnum::BEGIN ||
      enableDotExport.mode == DotExportSelection::DotExportEnum::ALL) {
    const auto sortedDot = pgis::config::GlobalConfig::get().getAs<bool>(sortDotEdges.cliName);
    metacg::io::dot::DotGenerator dotGenerator(consumer.getCallgraph(&consumer), sortedDot);
    dotGenerator.generate();
    dotGenerator.output({"./DotOutput", "PGIS-Dot", "begin"});
  }
  if (consumer.hasPassesRegistered()) {
    if (enableDotExport.mode == DotExportSelection::DotExportEnum::ALL) {
      consumer.setOutputDotBetweenPhases();
    }
    console->info("Running registered estimator phases");
    consumer.applyRegisteredPhases();
  }
  if (enableDotExport.mode == DotExportSelection::DotExportEnum::END ||
      enableDotExport.mode == DotExportSelection::DotExportEnum::ALL) {
    const auto sortedDot = pgis::config::GlobalConfig::get().getAs<bool>(sortDotEdges.cliName);
    metacg::io::dot::DotGenerator dotGenerator(consumer.getCallgraph(&consumer), sortedDot);
    dotGenerator.generate();
    dotGenerator.output({"./DotOutput", "PGIS-Dot", "end"});
  }

  // Serialize the cg
  {
    metacg::io::JsonSink jsSink;
    metacg::io::VersionTwoMCGWriter mcgw;
    mcgw.write(jsSink);
    const std::string filename = c.outputFile + "/instrumented-" + c.appName + ".mcg";
    std::ofstream ofile(filename);
    jsSink.output(ofile);
  }

  return metacg::pgis::SUCCESS;
}

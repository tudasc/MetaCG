/**
 * File: PGISMain.cpp
 * License: Part of the MetaCG project. Licensed under BSD 3 clause license. See LICENSE.txt file at
 * https://github.com/tudasc/metacg/LICENSE.txt
 */

#include "config/GlobalConfig.h"
#include "config/ParameterConfig.h"

#include "CubeReader.h"
#include "DotIO.h"
#include "DotReader.h"
#include "ErrorCodes.h"
#include "ExtrapEstimatorPhase.h"
#include "IPCGEstimatorPhase.h"
#include "LegacyMCGReader.h"
#include "LoggerUtil.h"
#include "MCGReader.h"
#include "MCGWriter.h"
#include "MetaData/PGISMetaData.h"
#include "PiraMCGProcessor.h"
#include <loadImbalance/LIEstimatorPhase.h>
#include <loadImbalance/LIMetaData.h>
#include <loadImbalance/OnlyMainEstimatorPhase.h>

#include "spdlog/spdlog.h"

#include "cxxopts.hpp"

#include <filesystem>

namespace pgis::options {

template <typename T>
auto optType(T &obj) {
  return cxxopts::value<arg_t<decltype(obj)>>();
}

}  // namespace pgis::options

using namespace pira;
using namespace ::pgis::options;

void registerEstimatorPhases(metacg::pgis::PiraMCGProcessor &cg, Config *c, bool isIPCG, float runtimeThreshold,
                             bool keepNotReachable, bool fillInstrumentationGaps) {
  auto statEstimator = new StatisticsEstimatorPhase(false, cg.getCallgraph());
  if (!keepNotReachable) {
    cg.registerEstimatorPhase(
        new RemoveUnrelatedNodesEstimatorPhase(cg.getCallgraph(), true, false));  // remove unrelated
  }
  cg.registerEstimatorPhase(statEstimator);

  // Actually do the selection
  if (!isIPCG) {
    metacg::MCGLogger::instance().getConsole()->info("New runtime threshold for profiling: ${}$", runtimeThreshold);
    cg.registerEstimatorPhase(new RuntimeEstimatorPhase(cg.getCallgraph(), runtimeThreshold));
  } else {
    HeuristicSelection::HeuristicSelectionEnum heuristicMode = pgis::config::getSelectedHeuristic();
    switch (heuristicMode) {
      case HeuristicSelection::HeuristicSelectionEnum::STATEMENTS: {
        const int nStmt = 2000;
        metacg::MCGLogger::instance().getConsole()->info("New runtime threshold for profiling: ${}$", nStmt);
        cg.registerEstimatorPhase(new StatementCountEstimatorPhase(nStmt, cg.getCallgraph(), true, statEstimator));
      } break;
      case HeuristicSelection::HeuristicSelectionEnum::CONDITIONALBRANCHES:
        cg.registerEstimatorPhase(new ConditionalBranchesEstimatorPhase(0, cg.getCallgraph(), statEstimator));
        break;
      case HeuristicSelection::HeuristicSelectionEnum::CONDITIONALBRANCHES_REVERSE:
        cg.registerEstimatorPhase(new ConditionalBranchesReverseEstimatorPhase(0, cg.getCallgraph(), statEstimator));
        break;
      case HeuristicSelection::HeuristicSelectionEnum::FP_MEM_OPS:
        cg.registerEstimatorPhase(new FPAndMemOpsEstimatorPhase(0, cg.getCallgraph(), statEstimator));
        break;
      case HeuristicSelection::HeuristicSelectionEnum::LOOPDEPTH:
        cg.registerEstimatorPhase(new LoopDepthEstimatorPhase(0, cg.getCallgraph(), statEstimator));
        break;
      case HeuristicSelection::HeuristicSelectionEnum::GlOBAL_LOOPDEPTH:
        cg.registerEstimatorPhase(new GlobalLoopDepthEstimatorPhase(0, cg.getCallgraph(), statEstimator));
        break;
    }
  }
  if (fillInstrumentationGaps) {
    cg.registerEstimatorPhase(new FillInstrumentationGapsPhase(cg.getCallgraph()));
  }
  cg.registerEstimatorPhase(new StatisticsEstimatorPhase(true, cg.getCallgraph()));
  cg.registerEstimatorPhase(new StoreInstrumentationDecisionsPhase(cg.getCallgraph()));
}

bool stringEndsWith(const std::string &s, const std::string &suffix) {
  return s.size() >= suffix.size() && s.compare(s.size() - suffix.size(), suffix.size(), suffix) == 0;
}

/**
 * Checks command line option for value, adds it to global config and returns the set value for local use.
 */
template <typename OptTy, typename ResTy>
typename OptTy::type storeOpt(const OptTy &opt, const ResTy &res) {
  using Ty = typename OptTy::type;
  Ty tmp = res[opt.cliName].template as<Ty>();
  pgis::config::GlobalConfig::get().putOption(opt.cliName, tmp);
  return tmp;
}

/**
 * Template only to use type deduction
 */
template <typename Opt>
void setDebugLevel(const Opt &result) {
  int printDebug = storeOpt(debugLevel, result);
  if (printDebug == 1) {
    spdlog::set_level(spdlog::level::debug);
  } else if (printDebug == 2) {
    spdlog::set_level(spdlog::level::trace);
  } else {
    spdlog::set_level(spdlog::level::info);
  }
}

int main(int argc, char **argv) {
  auto console = metacg::MCGLogger::instance().getConsole();
  auto errconsole = metacg::MCGLogger::instance().getErrConsole();

  if (argc == 1) {
    errconsole->error("Too few arguments. Use --help to show help.");
    exit(pgis::TooFewProgramArguments);
  }

  auto &gConfig = pgis::config::GlobalConfig::get();
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
    //("a,all-threads","Show all Threads even if unused.", cxxopts::value<bool>()->default_value("false"))
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
    (keepUnreachable.cliName, "Also consider functions which seem to be unreachable from main", optType(keepUnreachable)->default_value("false"))
    (fillGaps.cliName, "Fills gaps in the cg of instrumented functions", optType(fillGaps)->default_value(fillGaps.defaultValue))
    (overheadSelection.cliName, "Algorithm to deal with to high overheads", optType(overheadSelection)->default_value(overheadSelection.defaultValue))
    (sortDotEdges.cliName, "Sort edges in DOT graph lexicographically", optType(sortDotEdges)->default_value(sortDotEdges.defaultValue));
  // clang-format on

  Config c;

  auto result = opts.parse(argc, argv);

  if (result.count("help")) {
    std::cout << opts.help() << "\n";
    return pgis::SUCCESS;
  }

  /* Initialize the debug level */
  setDebugLevel(result);

  /* Enable DOT export */
  DotExportSelection enableDotExport;
  storeOpt(dotExport, result);
  storeOpt(sortDotEdges, result);

  /* Enable static, aggregated statement filtering */
  bool applyStaticFilter = storeOpt(staticSelection, result);

  /* Use Extra-P performance-model-based filtering or runtime-based only */
  bool applyModelFilter = storeOpt(modelFilter, result);
  bool extrapRuntimeOnly = storeOpt(runtimeOnly, result);
  storeOpt(extrapConfig, result);

  /* Whether profiling info should be exported */
  bool shouldExport = storeOpt(ipcgExport, result);

  /* Whether Score-P output file format should be used */
  bool useScorepFormat = storeOpt(scorepOut, result);
  if (useScorepFormat) {
    console->info("Setting Score-P Output Format");
  }

  /* Enable MPI Load-Imbalance detection */
  bool enableLide = storeOpt(lideEnabled, result);
  storeOpt(parameterFileConfig, result);

  /* Whether gaps in the instrumentation should be filled by a separate pass */
  bool fillInstrumentationGaps = storeOpt(fillGaps, result);

  /* Which MetaCG file format version to use/expect */
  int mcgVersion = storeOpt(metacgFormat, result);

  /* Enable call-site instrumentation (TODO: Is the instrumentor functional?) */
  storeOpt(useCallSiteInstrumentation, result);

  /* Only select nodes for instrumentation that can actually be instrumented */
  storeOpt(onlyInstrumentEligibleNodes, result);

  /* Keep unreachable nodes in the call graph */
  bool keepNotReachable = storeOpt(keepUnreachable, result);

  /* Select algorithms to use in the static selection */
  storeOpt(heuristicSelection, result);
  storeOpt(cuttoffSelection, result);

  /* Select targeted overhead */
  int targetOverheadArg = storeOpt(targetOverhead, result);
  int prevOverheadArg = storeOpt(prevOverhead, result);
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

  // for static instrumentation
  std::string mcgFullPath(argv[argc - 1]);
  // TODO: Use the filesystem STL functions for all of the path bits
  std::filesystem::path p(argv[argc - 1]);
  console->info("MetaCG file path given: {}", p.string());

  std::string mcgFilename = mcgFullPath.substr(mcgFullPath.find_last_of('/') + 1);
  c.appName = mcgFilename.substr(0, mcgFilename.find_last_of('.'));

  const auto parseExtrapArgs = [](auto argsRes) {
    if (argsRes.count("extrap")) {
      // Read in extra-p configuration
      auto filePath = pgis::config::GlobalConfig::get().getVal(extrapConfig);
      std::ifstream epFile(filePath);
      return extrapconnection::getExtrapConfigFromJSON(filePath);
    } else {
      return extrapconnection::ExtrapConfig();
    }
  };

  float runTimeThreshold = .0f;
  auto &cg = metacg::pgis::PiraMCGProcessor::get();
  auto &mcgm = metacg::graph::MCGManager::get();
  mcgm.addToManagedGraphs("emptyGraph", std::make_unique<metacg::Callgraph>());
  cg.setConfig(&c);
  cg.setExtrapConfig(parseExtrapArgs(result));

  /* To briefly inspect a CUBE profile for inclusive runtime of main */
  if (storeOpt(cubeShowOnly, result)) {
    if (result.count("cube")) {
      // for dynamic instrumentation
      std::string filePath(result["cube"].as<std::string>());

      std::string fileName = filePath.substr(filePath.find_last_of('/') + 1);

      if (stringEndsWith(filePath, ".cubex")) {
        CubeCallgraphBuilder::buildFromCube(filePath, &c, mcgm);
      } else if (stringEndsWith(filePath, ".dot")) {
        DOTCallgraphBuilder::build(filePath, &c);
      } else {
        errconsole->error("Unknown file ending in {}", filePath);
        exit(pgis::UnknownFileFormat);
      }
    }
    cg.printMainRuntime();
    return pgis::SUCCESS;
  }

  if (stringEndsWith(mcgFullPath, ".ipcg") || stringEndsWith(mcgFullPath, ".mcg")) {
    metacg::io::FileSource fs(mcgFullPath);
    if (::pgis::config::GlobalConfig::get().getVal(metacgFormat) == 1) {
      metacg::io::VersionOneMetaCGReader mcgReader(fs);
      mcgReader.read(mcgm);
    } else if (mcgVersion == 2) {
      metacg::io::VersionTwoMetaCGReader mcgReader(fs);
      mcgReader.read(mcgm);
      // XXX Find better way to do this: both conceptually and complexity-wise
      pgis::attachMetaDataToGraph<pira::PiraOneData>(mcgm.getCallgraph());
      pgis::attachMetaDataToGraph<pira::BaseProfileData>(mcgm.getCallgraph());
      pgis::attachMetaDataToGraph<LoadImbalance::LIMetaData>(mcgm.getCallgraph());
      pgis::attachMetaDataToGraph<pira::PiraTwoData>(mcgm.getCallgraph());
    }

    console->info("Read MetaCG with {} nodes.", mcgm.getCallgraph()->size());
    cg.setCG(mcgm.getCallgraph());

    if (applyStaticFilter) {
      // load imbalance detection
      // ========================
      if (enableLide) {
        console->info("Using trivial static analysis for load imbalance detection (OnlyMainEstimatorPhase");
        // static instrumentation -> OnlyMainEstimatorPhase
        if (!result.count("cube")) {
          cg.registerEstimatorPhase(new LoadImbalance::OnlyMainEstimatorPhase(cg.getCallgraph()));
          cg.applyRegisteredPhases();

          return pgis::SUCCESS;
        }
      } else {
        registerEstimatorPhases(cg, &c, true, 0, keepNotReachable, fillInstrumentationGaps);
        cg.applyRegisteredPhases();
        cg.removeAllEstimatorPhases();
      }
    }
  }
  cg.registerEstimatorPhase(new AttachInstrumentationResultsEstimatorPhase(cg.getCallgraph()));
  if (result.count("cube")) {
    // for dynamic instrumentation
    std::string filePath(result["cube"].as<std::string>());

    std::string fileName = filePath.substr(filePath.find_last_of('/') + 1);

    if (stringEndsWith(filePath, ".cubex")) {
      CubeCallgraphBuilder::buildFromCube(filePath, &c, mcgm);
    } else if (stringEndsWith(filePath, ".dot")) {
      DOTCallgraphBuilder::build(filePath, &c);
    } else {
      errconsole->error("Unknown file ending in {}", filePath);
      exit(pgis::UnknownFileFormat);
    }

    // load imbalance detection
    // ========================
    if (enableLide) {
      console->info("Using load imbalance detection mode");
      auto &pConfig = pgis::config::ParameterConfig::get();

      if (!pConfig.getLIConfig()) {
        errconsole->error(
            "Provide configuration for load imbalance detection. Refer to PIRA's README for further details.");
        return pgis::ErroneousHeuristicsConfiguration;
      }

      cg.registerEstimatorPhase(new LoadImbalance::LIEstimatorPhase(
          std::move(pConfig.getLIConfig()), cg.getCallgraph()));  // attention: moves out liConfig!

      cg.applyRegisteredPhases();

      // should be set for working load imbalance detection
      if (result.count("export")) {
        console->info("Exporting load imbalance data to IPCG file {}.", mcgFullPath);
        console->warn(
            "The old annotate mechanism has been removed and this functionality has not been tested with MCGWriter.");

        metacg::io::MCGWriter mcgWriter{mcgm};
        metacg::io::JsonSink jsonSink;
        mcgWriter.write(jsonSink);
        {
          std::ofstream out(mcgFullPath);
          out << jsonSink.getJson().dump(4) << std::endl;
        }

      } else {
        console->warn("--export flag is highly recommended for load imbalance detection");
      }

      return pgis::SUCCESS;
    } else {
      c.totalRuntime = c.actualRuntime;
      /* This runtime threshold currently unused */
      registerEstimatorPhases(cg, &c, false, runTimeThreshold, keepNotReachable, fillInstrumentationGaps);
      console->info("Registered estimator phases");
    }
  }

  if (result["extrap"].count()) {
    // test whether PIRA II configPtr is present
    auto &pConfig = pgis::config::ParameterConfig::get();
    if (!pConfig.getPiraIIConfig()) {
      console->error("Provide PIRA II configuration in order to use Extra-P estimators.");
      return pgis::ErroneousHeuristicsConfiguration;
    }

    cg.attachExtrapModels();

    if (applyModelFilter) {
      console->info("Applying model filter");
      cg.registerEstimatorPhase(
          new pira::ExtrapLocalEstimatorPhaseSingleValueFilter(cg.getCallgraph(), true, extrapRuntimeOnly));
    } else {
      console->info("Applying model expander");
      if (!keepNotReachable) {
        cg.registerEstimatorPhase(
            new RemoveUnrelatedNodesEstimatorPhase(cg.getCallgraph(), true, false));  // remove unrelated
      }
      cg.registerEstimatorPhase(
          new pira::ExtrapLocalEstimatorPhaseSingleValueExpander(cg.getCallgraph(), true, extrapRuntimeOnly));
    }
    if (fillInstrumentationGaps) {
      cg.registerEstimatorPhase(new FillInstrumentationGapsPhase(cg.getCallgraph()));
    }
    // XXX Should this be done after filter / expander were run? Currently we do this after model creation, yet,
    // *before* running the estimator phase
    if (result.count("export")) {
      console->info("Exporting to IPCG file.");
      console->warn(
          "The old annotate mechanism has been removed and this functionality has not been tested with MCGWriter.");

      metacg::io::MCGWriter mcgWriter{mcgm};
      metacg::io::JsonSink jsonSink;
      mcgWriter.write(jsonSink);
      {
        std::ofstream out(mcgFullPath);
        out << jsonSink.getJson().dump(4) << std::endl;
      }
    }
  }

  if (enableDotExport.mode == DotExportSelection::DotExportEnum::BEGIN ||
      enableDotExport.mode == DotExportSelection::DotExportEnum::ALL) {
    const auto sortedDot = pgis::config::GlobalConfig::get().getAs<bool>(sortDotEdges.cliName);
    metacg::io::dot::DotGenerator dotGenerator(cg.getCallgraph(&cg), sortedDot);
    dotGenerator.generate();
    dotGenerator.output({"./DotOutput", "PGIS-Dot", "begin"});
    //    cg.printDOT("begin");
  }
  if (cg.hasPassesRegistered()) {
    if (enableDotExport.mode == DotExportSelection::DotExportEnum::ALL) {
      cg.setOutputDotBetweenPhases();
    }
    console->info("Running registered estimator phases");
    cg.applyRegisteredPhases();
  }
  if (enableDotExport.mode == DotExportSelection::DotExportEnum::END ||
      enableDotExport.mode == DotExportSelection::DotExportEnum::ALL) {
    const auto sortedDot = pgis::config::GlobalConfig::get().getAs<bool>(sortDotEdges.cliName);
    metacg::io::dot::DotGenerator dotGenerator(cg.getCallgraph(&cg), sortedDot);
    dotGenerator.generate();
    dotGenerator.output({"./DotOutput", "PGIS-Dot", "end"});
    //    cg.printDOT("end");
  }

  // Serialize the cg
  {
    metacg::io::JsonSink jsSink;
    metacg::io::MCGWriter mcgw(mcgm);
    mcgw.write(jsSink);
    std::string filename = c.outputFile + "/instrumented-" + c.appName + ".mcg";
    std::ofstream ofile(filename);
    jsSink.output(ofile);
  }

  return pgis::SUCCESS;
}

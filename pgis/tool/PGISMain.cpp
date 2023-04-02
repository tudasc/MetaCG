/**
 * File: PGISMain.cpp
 * License: Part of the MetaCG project. Licensed under BSD 3 clause license. See LICENSE.txt file at
 * https://github.com/tudasc/metacg/LICENSE.txt
 */

#include "config/GlobalConfig.h"
#include "config/ParameterConfig.h"

#include "CubeReader.h"
#include "DotReader.h"
#include "LoggerUtil.h"
#include "MCGReader.h"
#include "MCGWriter.h"

#include "DotIO.h"
#include "ErrorCodes.h"
#include "ExtrapEstimatorPhase.h"
#include "IPCGEstimatorPhase.h"
#include "LegacyMCGReader.h"
#include "PiraMCGProcessor.h"
#include <loadImbalance/LIEstimatorPhase.h>
#include <loadImbalance/LIMetaData.h>
#include <loadImbalance/OnlyMainEstimatorPhase.h>

#include "spdlog/sinks/stdout_color_sinks.h"
#include "spdlog/spdlog.h"

#include "cxxopts.hpp"


namespace pgis::options {

template <typename T>
auto optType(T &obj) {
  return cxxopts::value<arg_t<decltype(obj)>>();
}
}  // namespace pgis::options

using namespace pira;
using namespace ::pgis::options;

void registerEstimatorPhases(metacg::pgis::PiraMCGProcessor &cg, Config *c, bool isIPCG, float runtimeThreshold,
                             bool keepNotReachable) {
  auto statEstimator = new StatisticsEstimatorPhase(false,cg.getCallgraph());
  if (!keepNotReachable) {
    cg.registerEstimatorPhase(new RemoveUnrelatedNodesEstimatorPhase(cg.getCallgraph(),true, false));  // remove unrelated
  }
  cg.registerEstimatorPhase(statEstimator);

  // Actually do the selection
  if (!isIPCG) {
    metacg::MCGLogger::instance().getConsole()->info("New runtime threshold for profiling: ${}$", runtimeThreshold);
    cg.registerEstimatorPhase(new RuntimeEstimatorPhase(cg.getCallgraph(),runtimeThreshold));
  } else {
    HeuristicSelection::HeuristicSelectionEnum heuristicMode = pgis::config::getSelectedHeuristic();
    switch (heuristicMode) {
      case HeuristicSelection::HeuristicSelectionEnum::STATEMENTS: {
        const int nStmt = 2000;
        metacg::MCGLogger::instance().getConsole()->info("New runtime threshold for profiling: ${}$", nStmt);
        cg.registerEstimatorPhase(new StatementCountEstimatorPhase(nStmt,   cg.getCallgraph(), true, statEstimator));
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

  cg.registerEstimatorPhase(new StatisticsEstimatorPhase(true, cg.getCallgraph()));
}

bool stringEndsWith(const std::string &s, const std::string &suffix) {
  return s.size() >= suffix.size() && s.compare(s.size() - suffix.size(), suffix.size(), suffix) == 0;
}

template <typename Target, typename OptsT, typename ConfigT>
void checkAndSet(const std::string id, const OptsT &opts, ConfigT &cfg) {
  auto &gConfig = pgis::config::GlobalConfig::get();
  if (opts.count(id)) {
    cfg = opts[id].template as<Target>();

    gConfig.putOption(id, cfg);
  } else {
    gConfig.putOption(id, opts[id].template as<Target>());
  }
}

int main(int argc, char **argv) {
  auto console = metacg::MCGLogger::instance().getConsole();
  auto errconsole = metacg::MCGLogger::instance().getErrConsole();

  if (argc == 1) {
    errconsole->error("Too few arguments. Use --help to show help.");
    exit(pgis::TooFewProgramArguments);
  }

  cxxopts::Options opts("PGIS", "Generating low-overhead instrumentation selections.");

  /* The marked options should go away for the PGIS release */
  // clang-format off
  opts.add_options()
    ("m,mangled", "Use mangled names", cxxopts::value<bool>()->default_value("false"))
    // not sure
    (outDirectory.cliName, "Output file name", optType(outDirectory)->default_value("out"))
    // from here: keep all
    (staticSelection.cliName, "Apply static selection", optType(staticSelection)->default_value("false"))
    ("c,cube", "Cube file for dynamic instrumentation", cxxopts::value<std::string>()->default_value(""))
    ("h,help", "Show help", cxxopts::value<bool>()->default_value("false"))
    (extrapConfig.cliName, "File to read Extra-P info from", optType(extrapConfig)->default_value(""))
    (modelFilter.cliName, "Use Extra-P models to filter only.", optType(modelFilter)->default_value("false"))
    (runtimeOnly.cliName, "Do not use model, but multiple runtimes", optType(runtimeOnly)->default_value("false"))
    ("a,all-threads","Show all Threads even if unused.", cxxopts::value<bool>()->default_value("false"))
    ("w, whitelist", "Filter nodes through given whitelist", cxxopts::value<std::string>()->default_value(""))
    (debugLevel.cliName, "Whether debug messages should be printed", optType(debugLevel)->default_value("0"))
    (scorepOut.cliName, "Write instrumentation file with Score-P syntax", optType(scorepOut)->default_value("false"))
    (ipcgExport.cliName, "Export the profiling info into IPCG file", optType(ipcgExport)->default_value("false"))
    (parameterFileConfig.cliName, "File path to configuration file containing analysis parameters", optType(parameterFileConfig)->default_value(""))
    (lideEnabled.cliName, "Enable load imbalance detection (PIRA LIDe)", optType(lideEnabled)->default_value("false"))
    (metacgFormat.cliName, "Selects the MetaCG format to expect", optType(metacgFormat)->default_value("1"))
    (dotExport.cliName, "Export call-graph as dot-file after every phase.", optType(dotExport)->default_value(dotExport.defaultValue))
    (cubeShowOnly.cliName, "Print inclusive time for main", optType(cubeShowOnly)->default_value("false"))
    (useCallSiteInstrumentation.cliName, "Enable experimental call-site instrumentation", optType(useCallSiteInstrumentation)->default_value("false"))
    (heuristicSelection.cliName, "Select the heuristic to use for node selection", optType(heuristicSelection)->default_value(heuristicSelection.defaultValue))
    (cuttoffSelection.cliName, "Select the algorithm to determine the cutoff for node selection", optType(cuttoffSelection)->default_value(cuttoffSelection.defaultValue))
    (keepUnreachable.cliName, "Also consider functions which seem to be unreachable from main", optType(keepUnreachable)->default_value("false"))
    (sortDotEdges.cliName, "Sort edges in DOT graph lexicographically", optType(sortDotEdges)->default_value(sortDotEdges.defaultValue));
  // clang-format on

  Config c;
  bool applyStaticFilter{false};
  bool applyModelFilter{false};
  bool shouldExport{false};
  bool useScorepFormat{false};
  bool extrapRuntimeOnly{false};
  bool enableLide{false};
  bool keepNotReachable{false};
  int printDebug{0};
  DotExportSelection enableDotExport;

  auto result = opts.parse(argc, argv);

  if (result.count("help")) {
    std::cout << opts.help() << "\n";
    return pgis::SUCCESS;
  }

  /* Options - populated into global configPtr */
  /* XXX These are currently only here for reference, while refactoring the options part of PGIS. */
  // checkAndSet<std::string>("other", result, c.otherPath);
  // checkAndSet<int>("samples", result, CgConfig::samplesPerSecond);
  // checkAndSet<double>("ref", result, c.referenceRuntime);
  // checkAndSet<bool>("mangled", result, c.useMangledNames);
  // checkAndSet<int>("half", result, c.nanosPerHalfProbe);
  // checkAndSet<bool>("tiny", result, c.tinyReport);
  // checkAndSet<bool>("ignore-sampling", result, c.ignoreSamplingOv);
  // checkAndSet<std::string>("samples-file", result, c.samplesFile);
  // checkAndSet<bool>("greedy-unwind", result, c.greedyUnwind);
  // checkAndSet<bool>("all-threads", result, c.showAllThreads);
  // checkAndSet<std::string>("whitelist", result, c.whitelist);

  checkAndSet<std::string>("out-file", result, c.outputFile);
  checkAndSet<bool>(staticSelection.cliName, result, applyStaticFilter);
  checkAndSet<bool>(modelFilter.cliName, result, applyModelFilter);
  checkAndSet<bool>(runtimeOnly.cliName, result, extrapRuntimeOnly);
  checkAndSet<int>(debugLevel.cliName, result, printDebug);
  checkAndSet<bool>(ipcgExport.cliName, result, shouldExport);
  checkAndSet<bool>(scorepOut.cliName, result, useScorepFormat);
  checkAndSet<DotExportSelection>(dotExport.cliName, result, enableDotExport);
  bool bDisposable;
  checkAndSet<bool>(sortDotEdges.cliName, result, bDisposable);
  std::string disposableStr;
  checkAndSet<std::string>(extrapConfig.cliName, result, disposableStr);
  checkAndSet<bool>(lideEnabled.cliName, result, enableLide);
  checkAndSet<std::string>(parameterFileConfig.cliName, result, disposableStr);

  int mcgVersion;
  checkAndSet<int>(metacgFormat.cliName, result, mcgVersion);

  checkAndSet<bool>(useCallSiteInstrumentation.cliName, result, bDisposable);

  checkAndSet<bool>(keepUnreachable.cliName, result, keepNotReachable);
  HeuristicSelection dispose_heuristic;
  checkAndSet<HeuristicSelection>(heuristicSelection.cliName, result, dispose_heuristic);

  if (mcgVersion < 2 &&
      pgis::config::getSelectedHeuristic() != HeuristicSelection::HeuristicSelectionEnum::STATEMENTS) {
    errconsole->error("Heuristics other than 'statements' are not supported with metacg format 1");
    exit(pgis::ErroneousHeuristicsConfiguration);
  }

  CuttoffSelection dispose_cuttoff;
  checkAndSet<CuttoffSelection>(cuttoffSelection.cliName, result, dispose_cuttoff);

  if (printDebug == 1) {
    spdlog::set_level(spdlog::level::debug);
  } else if (printDebug == 2) {
    spdlog::set_level(spdlog::level::trace);
  } else {
    spdlog::set_level(spdlog::level::info);
  }

  // for static instrumentation
  std::string mcgFullPath(argv[argc - 1]);
  std::string mcgFilename = mcgFullPath.substr(mcgFullPath.find_last_of('/') + 1);
  c.appName = mcgFilename.substr(0, mcgFilename.find_last_of('.'));

  const auto parseExtrapArgs = [](auto argsRes) {
    if (argsRes.count("extrap")) {
      // Read in extra-p configuration
      auto &gConfig = pgis::config::GlobalConfig::get();
      auto filePath = gConfig.getAs<std::string>(extrapConfig.cliName);
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
  if (result.count(cubeShowOnly.cliName)) {
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

  if (result.count("scorep-out")) {
    console->info("Setting Score-P Output Format");
  }

  if (stringEndsWith(mcgFullPath, ".ipcg") || stringEndsWith(mcgFullPath, ".mcg")) {
    metacg::io::FileSource fs(mcgFullPath);
    if (::pgis::config::GlobalConfig::get().getAs<int>(metacgFormat.cliName) == 1) {
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
          cg.registerEstimatorPhase(new LoadImbalance::OnlyMainEstimatorPhase());
          cg.applyRegisteredPhases();

          return pgis::SUCCESS;
        }
      } else {
        registerEstimatorPhases(cg, &c, true, 0, keepNotReachable);
        cg.applyRegisteredPhases();
        cg.removeAllEstimatorPhases();
      }
    }
  }

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
      auto &gConfig = pgis::config::GlobalConfig::get();
      auto &pConfig = pgis::config::ParameterConfig::get();

      if (!pConfig.getLIConfig()) {
        errconsole->error(
            "Provide configuration for load imbalance detection. Refer to PIRA's README for further details.");
        return pgis::ErroneousHeuristicsConfiguration;
      }

      cg.registerEstimatorPhase(new LoadImbalance::LIEstimatorPhase(std::move(pConfig.getLIConfig()),cg.getCallgraph())); // attention: moves out liConfig!

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
      registerEstimatorPhases(cg, &c, false, runTimeThreshold, keepNotReachable);
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
      cg.registerEstimatorPhase(new pira::ExtrapLocalEstimatorPhaseSingleValueFilter(cg.getCallgraph(),true, extrapRuntimeOnly));
    } else {
      console->info("Applying model expander");
      if (!keepNotReachable) {
        cg.registerEstimatorPhase(new RemoveUnrelatedNodesEstimatorPhase(cg.getCallgraph(),true, false));  // remove unrelated
      }
      cg.registerEstimatorPhase(new pira::ExtrapLocalEstimatorPhaseSingleValueExpander(cg.getCallgraph(),true, extrapRuntimeOnly));
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

  // Example use of MetaCG writer
  {
    metacg::io::JsonSink jsSink;
    metacg::io::MCGWriter mcgw(mcgm);
    mcgw.write(jsSink);
    std::ofstream ofile("filename");
    jsSink.output(ofile);
  }

  return pgis::SUCCESS;
}

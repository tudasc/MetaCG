/**
 * File: PGISMain.cpp
 * License: Part of the MetaCG project. Licensed under BSD 3 clause license. See LICENSE.txt file at https://github.com/tudasc/pira/LICENSE.txt
 */


#include "GlobalConfig.h"

#include "CubeReader.h"
#include "DotReader.h"
#include "MCGReader.h"

#include "Callgraph.h"

#include "ExtrapEstimatorPhase.h"
#include "IPCGEstimatorPhase.h"
#include <loadImbalance/LIRetriever.h>
#include <loadImbalance/OnlyMainEstimatorPhase.h>
#include <loadImbalance/LIEstimatorPhase.h>

#include "spdlog/spdlog.h"
#include "spdlog/sinks/stdout_color_sinks.h"

#include "cxxopts.hpp"

#include <fstream>
#include <nlohmann/json.hpp>
#include <vector>

namespace pgis::options {

    template <typename T>
      auto optType(T &obj) {
        return cxxopts::value< arg_t<decltype(obj)> >();
      }
}

using namespace pira;
using namespace pgis::options;
   

void registerEstimatorPhases(CallgraphManager &cg, Config *c, bool isIPCG, float runtimeThreshold) {
  auto statEstimator = new StatisticsEstimatorPhase(false);
  cg.registerEstimatorPhase(new RemoveUnrelatedNodesEstimatorPhase(true, false));  // remove unrelated
  cg.registerEstimatorPhase(new ResetEstimatorPhase());
  cg.registerEstimatorPhase(statEstimator);
  cg.registerEstimatorPhase(new ResetEstimatorPhase());

  // Actually do the selection
  if (!isIPCG) {
    spdlog::get("console")->info("New runtime threshold for profiling: ${}$", runtimeThreshold);
    cg.registerEstimatorPhase(new RuntimeEstimatorPhase(runtimeThreshold));
  } else {
    const int nStmt = 2000;
    spdlog::get("console")->info("New statement threshold for profiling: ${}$", nStmt);
    cg.registerEstimatorPhase(new StatementCountEstimatorPhase(nStmt, true, statEstimator));
  }

  cg.registerEstimatorPhase(new StatisticsEstimatorPhase(true));
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
  auto console = spdlog::stdout_color_mt("console");
  auto errconsole= spdlog::stderr_color_mt("errconsole");

  if (argc == 1) {
    spdlog::get("errconsole")->error("Too few arguments. Use --help to show help.");
    exit(-1);
  }

  cxxopts::Options opts("PGIS", "Generating low-overhead instrumentation selections.");

  /* The marked options should go away for the PGIS release */
  // clang-format off
  opts.add_options()
    // not sure
    ("other", "", cxxopts::value<std::string>()->default_value(""))
    // remove
    ("s,samples", "Samples per second", cxxopts::value<long>()->default_value("0"))
    // remove
    ("r,ref", "??", cxxopts::value<double>()->default_value("0"))
    // not sure
    ("m,mangled", "Use mangled names", cxxopts::value<bool>()->default_value("false"))
    // remove
    ("half", "??", cxxopts::value<long>()->default_value("0"))
    // not sure
    ("t,tiny", "Print tiny report", cxxopts::value<bool>()->default_value("false"))
    // remove
    ("i,ignore-sampling", "Ignore sampling", cxxopts::value<bool>()->default_value("false"))
    // remove
    ("f,samples-file", "Input file for sampling points", cxxopts::value<std::string>())
    // remove
    ("g,greedy-unwind", "Use greedy unwind", cxxopts::value<bool>()->default_value("false"))
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
    (loadImbalanceConfig.cliName, "File path to configuration file for load imbalance detection", optType(loadImbalanceConfig)->default_value(""))
    (metacgFormat.cliName, "Selects the MetaCG format to expect", optType(metacgFormat)->default_value("1"));
  // clang-format on

  Config c;
  bool applyStaticFilter {false};
  bool applyModelFilter {false};
  bool shouldExport {false};
  bool useScorepFormat {false};
  bool extrapRuntimeOnly {false};
  int printDebug{0};

  auto result = opts.parse(argc, argv);

  if (result.count("help")) {
    std::cout << opts.help() << "\n";
    return 0;
  }

  /* Options - populated into global config */
  /* XXX These are currently only here for reference, while refactoring the options part of PGIS. */
  //checkAndSet<std::string>("other", result, c.otherPath);
  //checkAndSet<int>("samples", result, CgConfig::samplesPerSecond);
  //checkAndSet<double>("ref", result, c.referenceRuntime);
  //checkAndSet<bool>("mangled", result, c.useMangledNames);
  //checkAndSet<int>("half", result, c.nanosPerHalfProbe);
  //checkAndSet<bool>("tiny", result, c.tinyReport);
  //checkAndSet<bool>("ignore-sampling", result, c.ignoreSamplingOv);
  //checkAndSet<std::string>("samples-file", result, c.samplesFile);
  //checkAndSet<bool>("greedy-unwind", result, c.greedyUnwind);
  //checkAndSet<bool>("all-threads", result, c.showAllThreads);
  //checkAndSet<std::string>("whitelist", result, c.whitelist);

  checkAndSet<std::string>("out-file", result, c.outputFile);

  checkAndSet<bool>(staticSelection.cliName, result, applyStaticFilter);
  checkAndSet<bool>(modelFilter.cliName, result, applyModelFilter);
  checkAndSet<bool>(runtimeOnly.cliName, result, extrapRuntimeOnly);
  checkAndSet<int>(debugLevel.cliName, result, printDebug);
  checkAndSet<bool>(ipcgExport.cliName, result, shouldExport);
  checkAndSet<bool>(scorepOut.cliName, result, useScorepFormat);
  std::string disposable;
  checkAndSet<std::string>(extrapConfig.cliName, result, disposable);
  checkAndSet<std::string>(loadImbalanceConfig.cliName, result, disposable);

  int mcgVersion;
  checkAndSet<int>(metacgFormat.cliName, result, mcgVersion);

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
  auto &cg = CallgraphManager::get();
  cg.setConfig(&c);
  cg.setExtrapConfig(parseExtrapArgs(result));
  
  if (result.count("scorep-out")) {
    spdlog::get("console")->info("Setting Score-P Output Format");
    cg.setScorepOutputFormat();
  }

  if (stringEndsWith(mcgFullPath, ".ipcg") || stringEndsWith(mcgFullPath, ".mcg")) {
    MetaCG::io::FileSource fs(mcgFullPath);
    if (pgis::config::GlobalConfig::get().getAs<int>(metacgFormat.cliName) == 1) {
      MetaCG::io::VersionOneMetaCGReader mcgReader(fs);
      mcgReader.read(cg);
    } else if (mcgVersion == 2) {
      MetaCG::io::VersionTwoMetaCGReader mcgReader(fs);
      // Example how to add MetaDataHandler
      cg.addMetaHandler<MetaCG::io::retriever::PiraOneDataRetriever>();
      cg.addMetaHandler<MetaCG::io::retriever::FilePropertyHandler>();
      cg.addMetaHandler<MetaCG::io::retriever::CodeStatisticsHandler>();
      cg.addMetaHandler<LoadImbalance::LoadImbalanceMetaDataHandler>();
      mcgReader.read(cg);
    }
    
    //MetaCG::io::buildFromJSON(cg, ipcgFullPath, &c);
    
    if (applyStaticFilter) {
      // load imbalance detection
      // ========================
      if(result.count(loadImbalanceConfig.cliName)) {
        spdlog::get("console")->info("Using trivial static analysis for load imbalance detection (OnlyMainEstimatorPhase");
        // static instrumentation -> OnlyMainEstimatorPhase
        if (!result.count("cube")) {
          cg.registerEstimatorPhase(new LoadImbalance::OnlyMainEstimatorPhase());
          cg.applyRegisteredPhases();

          return EXIT_SUCCESS;
        }
      } else {
        registerEstimatorPhases(cg, &c, true, 0);
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
      CubeCallgraphBuilder::buildFromCube(filePath, &c, cg);
    } else if (stringEndsWith(filePath, ".dot")) {
      DOTCallgraphBuilder::build(filePath, &c);
    } else {
      spdlog::get("errconsole")->error("Unknown file ending in {}", filePath);
      exit(-1);
    }

    // load imbalance detection
    // ========================
    if(result.count(loadImbalanceConfig.cliName)) {
      spdlog::get("console")->info("Using load imbalance detection mode");
      auto &gConfig = pgis::config::GlobalConfig::get();
      try {
        LoadImbalance::Config liConfig =
            LoadImbalance::Config::generateFromJSON(gConfig.getAs<std::string>(loadImbalanceConfig.cliName));
        cg.registerEstimatorPhase(new LoadImbalance::LIEstimatorPhase(liConfig));
      } catch(nlohmann::json::exception& e) {
        spdlog::get("errconsole")->error("Unable to parse load imbalance configuration file: {}", e.what());
        return EXIT_FAILURE;
      }

      cg.applyRegisteredPhases();

      // should be set for working load imbalance detection
      if (result.count("export")) {
        console->info("Exporting load imbalance data to IPCG file.");
        MetaCG::io::annotateJSON(cg.getCallgraph(&CallgraphManager::get()), mcgFullPath, LoadImbalance::LIRetriever());
      } else {
        spdlog::get("console")->warn("--export-flag is highly recommended for load imbalance detection");
      }

      return EXIT_SUCCESS;
    } else {
      c.totalRuntime = c.actualRuntime;
      /* This runtime threshold currently unused */
      registerEstimatorPhases(cg, &c, false, runTimeThreshold);
      console->info("Registered estimator phases");
    }
  }

  if (result["extrap"].count()) {
    cg.attachExtrapModels();
    cg.printDOT("extrap");

    if (applyModelFilter) {
      console->info("Applying model filter");
      cg.registerEstimatorPhase(new pira::ExtrapLocalEstimatorPhaseSingleValueFilter(1.0, true, extrapRuntimeOnly));
    } else {
      console->info("Applying model expander");
      cg.registerEstimatorPhase(new RemoveUnrelatedNodesEstimatorPhase(true, false));  // remove unrelated
      cg.registerEstimatorPhase(new pira::ExtrapLocalEstimatorPhaseSingleValueExpander(1.0, true, extrapRuntimeOnly));
    }
    // XXX Should this be done after filter / expander were run? Currently we do this after model creation, yet, *before*
    // running the estimator phase
    if (result.count("export")) {
      console->info("Exporting to IPCG file.");
      MetaCG::io::annotateJSON(cg.getCallgraph(&CallgraphManager::get()), mcgFullPath, MetaCG::io::retriever::PiraTwoDataRetriever());
   }
  }

  if (cg.hasPassesRegistered()) {
    spdlog::get("console")->info("Running registered estimator phases");
    cg.applyRegisteredPhases();
  }

  return EXIT_SUCCESS;
}

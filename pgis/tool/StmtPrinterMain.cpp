#include <cstdlib>
#include <fstream>
#include <vector>

#include "CubeReader.h"
#include "DotReader.h"
#include "IPCGReader.h"

#include "Callgraph.h"

#include "EdgeBasedOptimumEstimatorPhase.h"
#include "IPCGEstimatorPhase.h"
#include "NodeBasedOptimumEstimatorPhase.h"
#include "ProximityMeasureEstimatorPhase.h"
#include "SanityCheckEstimatorPhase.h"

void registerEstimatorPhases(CallgraphManager &cg, Config *c, bool isIPCG, float runtimeThreshold) {
  auto statEstimator = new StatisticsEstimatorPhase(false);  // used to compute values for other phases
  cg.registerEstimatorPhase(new RemoveUnrelatedNodesEstimatorPhase(true, false));  // remove unrelated
  cg.registerEstimatorPhase(new ResetEstimatorPhase());
  cg.registerEstimatorPhase(statEstimator);
  cg.registerEstimatorPhase(new ResetEstimatorPhase());

  // Actually do the selection
  if (!isIPCG) {
    std::cout << "New threshold runtime for profiling: " << runtimeThreshold << std::endl;
    cg.registerEstimatorPhase(new RuntimeEstimatorPhase(runtimeThreshold));
  } else {
    const int nStmt = 2000;
    std::cout << "[PGIS] [STATIC] $" << nStmt << "$" << std::endl;
    cg.registerEstimatorPhase(new StatementCountEstimatorPhase(nStmt, true, statEstimator));
  }

  cg.registerEstimatorPhase(new StatisticsEstimatorPhase(true));  // used to print statistics
  cg.registerEstimatorPhase(new ResetEstimatorPhase());
}

bool stringEndsWith(const std::string &s, const std::string &suffix) {
  return s.size() >= suffix.size() && s.compare(s.size() - suffix.size(), suffix.size(), suffix) == 0;
}

int main(int argc, char **argv) {
  if (argc == 1) {
    std::cerr << "ERROR: too few arguments." << std::endl;
    exit(-1);
  }

  Config c;
  for (int i = argc; i < argc; ++i) {
    auto arg = std::string(argv[i]);

    if (arg == "--other") {
      c.otherPath = std::string(argv[++i]);
      continue;
    }
    if (arg == "--samples" || arg == "-s") {
      CgConfig::samplesPerSecond = atoi(argv[++i]);
      continue;
    }
    if (arg == "--ref" || arg == "-r") {
      c.referenceRuntime = atof(argv[++i]);
      continue;
    }
    if (arg == "--mangled" || arg == "-m") {
      c.useMangledNames = true;
      continue;
    }
    if (arg == "--half" || arg == "-h") {
      c.nanosPerHalfProbe = atoi(argv[++i]);
      continue;
    }
    if (arg == "--tiny" || arg == "-t") {
      c.tinyReport = true;
      continue;
    }
    if (arg == "--ignore-sampling" || arg == "-i") {
      c.ignoreSamplingOv = true;
      continue;
    }
    if (arg == "--samples-file" || arg == "-f") {
      c.samplesFile = "active";  // ugly hack
      continue;
    }
    if (arg == "--greedy-Unwind" || arg == "-g") {
      c.greedyUnwind = true;
      continue;
    }
    if (arg == "--out-file") {
      c.outputFile = std::string(argv[++i]);
      continue;
    }

    std::cerr << "Unknown option: " << argv[i] << std::endl;

    std::cout << "Usage: " << argv[0] << " /PATH/TO/CUBEX/PROFILE"
              << " [--other|-o /PATH/TO/PROFILE/TO/COMPARE/TO]"
              << " [--samples|-s NUMBER_OF_SAMPLES_PER_SECOND]"
              << " [--ref|-r UNINSTRUMENTED_RUNTIME_SECONDS]"
              << " [--half|-h NANOS_FOR_OVERHEAD_COMPENSATION"
              << " [--mangled|-m]"
              << " [--tiny|-t]" << std::endl
              << std::endl;
  }

  // for static instrumentation
  std::string filePath_ipcg(argv[1]);
  std::string ipcg_fileName = filePath_ipcg.substr(filePath_ipcg.find_last_of('/') + 1);
  c.appName = ipcg_fileName.substr(0, ipcg_fileName.find_last_of('.'));

  float runTimeThreshold{.0f};
  CallgraphManager cg(&c);

  if (stringEndsWith(filePath_ipcg, ".ipcg")) {
    std::cout << "Reading from ipcg file " << filePath_ipcg << std::endl;
    IPCGAnal::buildFromJSON(cg, filePath_ipcg, &c);
    if (argc == 2) {
      registerEstimatorPhases(cg, &c, true, 0);
    }
    cg.applyRegisteredPhases();
  }

  if (argc > 2) {
    // for dynamic instrumentation
    std::string filePath(argv[2]);
    std::string fileName = filePath.substr(filePath.find_last_of('/') + 1);
    c.appName = fileName.substr(0, fileName.find_last_of('.'));  // remove .*

    if (!c.samplesFile.empty()) {
      c.samplesFile = filePath.substr(0, filePath.find_last_of('.')) + ".samples";
      std::cout << c.samplesFile << std::endl;
    }

    if (stringEndsWith(filePath, ".cubex")) {
      CubeCallgraphBuilder::buildFromCube(filePath, &c, cg);
      // smRTT = CubeCallgraphBuilder::CalculateRuntimeThreshold(&cg);
    } else if (stringEndsWith(filePath, ".dot")) {
      cg = DOTCallgraphBuilder::build(filePath, &c);
    } else {
      std::cerr << "ERROR: Unknown file ending in " << filePath << std::endl;
      exit(-1);
    }

    c.totalRuntime = c.actualRuntime;
    /* This runtime threshold currently unused */
    registerEstimatorPhases(cg, &c, false, runTimeThreshold);
    std::cout << "Registered estimator phases.\n";
    cg.applyRegisteredPhases();
  }

  return EXIT_SUCCESS;
}

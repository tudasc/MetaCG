#include "CubeReader.h"

#include "spdlog/spdlog.h"

using namespace pira;

void CubeCallgraphBuilder::build(std::string filePath, Config *c) {
  //CallgraphManager *cg = new CallgraphManager(c, {});

  auto &cg = CallgraphManager::get();
  cg.setConfig(c);

  try {
    // get logger
    auto console = spdlog::get("console");
    // Create cube instance
    cube::Cube cube;
    // Read our cube file
    console->info("Reading cube file {}", filePath);
    cube.openCubeReport(filePath);
    // Get the cube nodes
    const std::vector<cube::Cnode *> &cnodes = cube.get_cnodev();
    unsigned long long overallNumberOfCalls = 0;
    double overallRuntime = 0.0;

    double smallestFunctionInSeconds = 1e9;
    std::string smallestFunctionName;
    int edgesWithZeroRuntime = 0;

    cube::Metric *timeMetric = cube.get_met("time");
    cube::Metric *visitsMetric = cube.get_met("visits");

    const std::vector<cube::Thread *> threads = cube.get_thrdv();

    for (auto cnode : cnodes) {
      // I don't know when this happens, but it does.
      if (cnode->get_parent() == nullptr) {
        cg.findOrCreateNode(
            c->useMangledNames ? cnode->get_callee()->get_mangled_name() : cnode->get_callee()->get_name(),
            cube.get_sev(timeMetric, cnode, threads.at(0)));
        continue;
      }

      // Put the parent/child pair into our call graph
      auto parentNode = cnode->get_parent()->get_callee();  // RN: don't trust no one. It IS the parent node
      auto childNode = cnode->get_callee();

      auto parentName = c->useMangledNames ? parentNode->get_mangled_name() : parentNode->get_name();
      auto childName = c->useMangledNames ? childNode->get_mangled_name() : childNode->get_name();

      for (unsigned int i = 0; i < threads.size(); i++) {
        unsigned long long numberOfCalls = (unsigned long long)cube.get_sev(visitsMetric, cnode, threads.at(i));
        double timeInSeconds = cube.get_sev(timeMetric, cnode, threads.at(i));

        cg.putEdge(parentName, parentNode->get_mod(), parentNode->get_begn_ln(), childName, numberOfCalls,
                    timeInSeconds, threads.at(i)->get_id(), threads.at(i)->get_parent()->get_id());

        overallNumberOfCalls += numberOfCalls;
        overallRuntime += timeInSeconds;

        double runtimePerCallInSeconds = timeInSeconds / numberOfCalls;
        if (runtimePerCallInSeconds < smallestFunctionInSeconds) {
          smallestFunctionInSeconds = runtimePerCallInSeconds;
          smallestFunctionName = childName;
        }
      }

      cg.setNodeComesFromCube(parentName);
      cg.setNodeComesFromCube(childName);
    }

    // read in samples per second TODO these are hardcoded for 10kHz
    auto samplesFilename = c->samplesFile;
    if (!samplesFilename.empty()) {
      std::ifstream inFile(samplesFilename);
      if (!inFile.is_open()) {
        std::cerr << "Can not open samples-file: " << samplesFilename << std::endl;
        exit(1);
      }
      std::string line;
      while (std::getline(inFile, line)) {
        std::string name;
        unsigned long long numberOfSamples;

        inFile >> numberOfSamples >> name;

        cg.putNumberOfSamples(name, numberOfSamples);
      }
    }

    c->actualRuntime = overallRuntime;
    bool hasRefTime = c->referenceRuntime > .0;
    unsigned long long normalProbeNanos = overallNumberOfCalls * CgConfig::nanosPerNormalProbe;

    double probeSeconds = (double(normalProbeNanos)) / (1000 * 1000 * 1000);
    double probePercent = probeSeconds / (overallRuntime - probeSeconds) * 100;
    if (hasRefTime) {
      probeSeconds = overallRuntime - c->referenceRuntime;
      probePercent = probeSeconds / c->referenceRuntime * 100;
    }

    std::cout << "####################### " << c->appName << " #######################" << std::endl;
    if (!hasRefTime) {
      std::cout << "HAS NO REF TIME" << std::endl;
    }
    std::cout << "    "
              << "numberOfCalls: " << overallNumberOfCalls << " | "
              << "samplesPerSecond : " << CgConfig::samplesPerSecond << std::endl
              << "    "
              << "runtime: " << overallRuntime << " s (ref " << c->referenceRuntime << " s)";
    std::cout << " | "
              << "overhead: " << (hasRefTime ? "" : "(est.) ") << probeSeconds << " s"
              << " or " << std::setprecision(4) << probePercent << " %" << std::endl;

    std::cout << "    smallestFunction : " << smallestFunctionName << " : " << smallestFunctionInSeconds * 1e9 << "ns"
              << " | edgesWithZeroRuntime: " << edgesWithZeroRuntime << std::setprecision(6) << std::endl
              << std::endl;

    //return cg;

  } catch (const cube::RuntimeError &e) {
    std::cout << "CubeReader failed: " << std::endl << e.get_msg() << std::endl;
    exit(-1);
  }
}

void CubeCallgraphBuilder::buildFromCube(std::string filePath, Config *c, CallgraphManager &cg) {
  try {
    // Create cube instance
    cube::Cube cube;
    // Read our cube file
    cube.openCubeReport(filePath);
    // Get the cube nodes
    const std::vector<cube::Cnode *> &cnodes = cube.get_cnodev();
    unsigned long long overallNumberOfCalls = 0;
    double overallRuntime = 0.0;

    double smallestFunctionInSeconds = 1e9;
    std::string smallestFunctionName;
    int edgesWithZeroRuntime = 0;

    cube::Metric *timeMetric = cube.get_met("time");
    cube::Metric *visitsMetric = cube.get_met("visits");

    const std::vector<cube::Thread *> threads = cube.get_thrdv();
    for (auto cnode : cnodes) {
      // This happens for static initializers
      if (cnode->get_parent() == nullptr) {
        auto fName = c->useMangledNames ? cnode->get_callee()->get_mangled_name() : cnode->get_callee()->get_name();
        cg.findOrCreateNode(fName, cube.get_sev(timeMetric, cnode, threads.at(0)));
        cg.setNodeComesFromCube(fName);
        continue;
      }

      // Put the parent/child pair into our call graph
      auto parentNode = cnode->get_parent()->get_callee();  // RN: don't trust no one. It IS the parent node
      auto childNode = cnode->get_callee();
      auto parentName = c->useMangledNames ? parentNode->get_mangled_name() : parentNode->get_name();
      auto childName = c->useMangledNames ? childNode->get_mangled_name() : childNode->get_name();

      for (unsigned int i = 0; i < threads.size(); ++i) {
        unsigned long long numberOfCalls = (unsigned long long)cube.get_sev(visitsMetric, cnode, threads.at(i));
        double timeInSeconds = cube.get_sev(timeMetric, cnode, threads.at(i));

        cg.putEdge(parentName, parentNode->get_mod(), parentNode->get_begn_ln(), childName, numberOfCalls,
                   timeInSeconds, threads.at(i)->get_id(), threads.at(i)->get_parent()->get_id());

        overallNumberOfCalls += numberOfCalls;
        overallRuntime += timeInSeconds;

        double runtimePerCallInSeconds = timeInSeconds / numberOfCalls;
        if (runtimePerCallInSeconds < smallestFunctionInSeconds) {
          smallestFunctionInSeconds = runtimePerCallInSeconds;
          smallestFunctionName = childName;
        }
      }

      cg.setNodeComesFromCube(parentName);
      cg.setNodeComesFromCube(childName);
    }

    c->actualRuntime = overallRuntime;

    std::cout << "####################### " << c->appName << " #######################" << std::endl;
    std::cout << "    "
              << "numberOfCalls: " << overallNumberOfCalls << " | "
              << "samplesPerSecond : " << CgConfig::samplesPerSecond << std::endl
              << "    "
              << "runtime: " << overallRuntime << " s (ref " << c->referenceRuntime << " s)";

    std::cout << "    smallestFunction : " << smallestFunctionName << " : " << smallestFunctionInSeconds * 1e9 << "ns"
              << " | edgesWithZeroRuntime: " << edgesWithZeroRuntime << std::setprecision(6) << std::endl
              << std::endl;

    // Check for nullptrs
    for (auto node : cg) {
      if (node == nullptr) {
        std::cout << "detected null pointer at construction" << std::endl;
        abort();
      }
    }

  } catch (const cube::RuntimeError &e) {
    std::cout << "CubeReader failed: " << std::endl << e.get_msg() << std::endl;
    exit(-1);
  }
}

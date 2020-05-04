#include "CubeReader.h"

#include "spdlog/spdlog.h"

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

float CubeCallgraphBuilder::CalculateRuntimeThreshold(CallgraphManager *cg) {
  int i = 0;
  Callgraph cgptr = cg->getCallgraph(cg);
  Callgraph::ContainerT cgptrset = cgptr.getGraph();
  int nodes_count = cgptrset.size();
  float data[nodes_count];
  for (auto it = std::begin(cgptrset); it != std::end(cgptrset); ++it) {
    auto funNode = *it;
    // std::cout << funNode->getFunctionName() << " -> " <<
    // funNode->getInclusiveRuntimeInSeconds() << "\n";
    if (funNode->getInclusiveRuntimeInSeconds() > 0) {
      data[i++] = funNode->getInclusiveRuntimeInSeconds();
    }
  }
  float ret_val = bucket_sort(data, i);
  return ret_val;
}

float CubeCallgraphBuilder::bucket_sort(float arr[], int n) {
  float minValue = arr[0];
  float maxValue = arr[0];

  for (int i = 0; i < n; i++) {
    if (arr[i] > maxValue)
      maxValue = arr[i];
    if (arr[i] < minValue)
      minValue = arr[i];
  }
  int bucketLength = (int)(maxValue - minValue + 1);
  std::vector<float> b[bucketLength];
  int bi = 0;

  // 2) Put array elements in different buckets
  for (int i = 0; i < n; i++) {
    /*if(arr[i] < 1){ bi = 0; }
    else if(arr[i] < 2){bi=1;}
    else if(arr[i] < 3){bi=2;}
    else if(arr[i] < 4){bi=3;}
    else if(arr[i] < 5){bi=4;}
    else{bi = 5;}*/
    int bi = (int)(arr[i] - minValue);  // Index in bucket
    b[bi].push_back(arr[i]);
  }

  // 3) Sort individual buckets
  for (int i = 0; i < bucketLength; i++)
    sort(b[i].begin(), b[i].end());

  int index = 0;
  for (int i = 0; i < bucketLength; i++)
    for (int j = 0; j < b[i].size(); j++)
      arr[index++] = b[i][j];

  int threshold_index = (90 * n) / 100;
  return arr[threshold_index];
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

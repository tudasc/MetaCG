/**
 * File: ExtrapConnection.cpp
 * License: Part of the MetaCG project. Licensed under BSD 3 clause license. See LICENSE.txt file at
 * https://github.com/tudasc/metacg/LICENSE.txt
 */

#include "ExtrapConnection.h"
#include "CubeReader.h"

#include "nlohmann/json.hpp"
#include "spdlog/spdlog.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wextra"
#pragma GCC diagnostic ignored "-Wpedantic"
#pragma GCC diagnostic ignored "-Wunused-variable"
#pragma GCC diagnostic ignored "-Wsign-compare"
#include "PiraMCGProcessor.h"
#include "EXTRAP_SingleParameterModelGenerator.hpp"
#include <EXTRAP_MultiParameterSimpleModelGenerator.hpp>
#include <EXTRAP_SingleParameterSimpleModelGenerator.hpp>
#include <config/PiraIIConfig.h>
#pragma GCC diagnostic pop

namespace extrapconnection {
void printConfig(ExtrapConfig &cfg) {
  auto console = spdlog::get("console");

  std::string parameterStr;
  for (const auto &p : cfg.params) {
    parameterStr += '(' + p.first + ", [";
    std::vector<int> pVals = p.second;
    for (const auto &v : pVals) {
      parameterStr += std::to_string(v) + ", ";
    }
    parameterStr += "])\n";
  }
  console->info(
      "---- Extra-P Config ----\nBaseDir: {}\nRepetitions: {}\nPrefix: {}\nPostfix: {}\nIterations: {}\nParams: "
      "{}\n---- End Extra-P Config ----",
      cfg.directory, cfg.repetitions, cfg.prefix, cfg.postfix, cfg.iteration, parameterStr);
}

ExtrapConfig getExtrapConfigFromJSON(std::string filePath) {
  using json = nlohmann::json;

  json j;
  {
    std::ifstream infile(filePath);
    infile >> j;
  }

  ExtrapConfig cfg;
  for (json::iterator it = j.begin(); it != j.end(); ++it) {
    auto key = it.key();
    spdlog::get("console")->debug("Iterating for {}", key);

    if (key == "dir") {
      cfg.directory = it.value().get<std::string>();
      continue;
    }
    if (key == "prefix") {
      cfg.prefix = it.value().get<std::string>();
      continue;
    }
    if (key == "postfix") {
      cfg.postfix = '.' + it.value().get<std::string>();
      continue;
    }
    if (key == "reps") {
      cfg.repetitions = it.value().get<int>();
      continue;
    }
    if (key == "iter") {
      cfg.iteration = it.value().get<int>();
      continue;
    }
    if (key == "params") {
      auto subTree = it.value();
      for (json::iterator st_it = subTree.begin(); st_it != subTree.end(); ++st_it) {
        // This case should only be hit, when actually a PARAMETER!
        std::string paramName = '.' + st_it.key();
        std::vector<std::string> paramStrs = st_it.value().get<std::vector<std::string>>();
        std::vector<int> paramValues;
        paramValues.reserve(paramStrs.size());
        std::transform(std::begin(paramStrs), std::end(paramStrs), std::back_inserter(paramValues), [](std::string &s) {
          spdlog::get("console")->debug("Transforming {}", s);
          return std::stoi(s);
        });
        cfg.params.emplace_back(std::make_pair(paramName, paramValues));
      }
    } else {
      spdlog::get("errconsole")->warn("This should not happen! Unkown json identifier found.");
    }
    std::reverse(std::begin(cfg.params), std::end(cfg.params));
    for (auto &p : cfg.params) {
      spdlog::get("console")->debug("{}", p.first);
    }
  }

  // XXX How to have this neat and tidy in a single statement?
  spdlog::get("console")->info("Parsed File. Resulting Config:");
  printConfig(cfg);

  return cfg;
}

const auto getKeys = [](const auto c) {
  std::vector<typename decltype(c)::value_type::first_type> keys;
  for (const auto &e : c) {
    if (std::find(std::begin(keys), std::end(keys), e.first) == std::end(keys)) {
      keys.push_back(e.first);
    }
  }
  return keys;
};

std::vector<EXTRAP::Parameter> ExtrapModelProvider::getParameterList() {
  std::vector<EXTRAP::Parameter> params;
  params.reserve(config.params.size());

  for (const auto &p : getKeys(config.params)) {
    params.emplace_back(EXTRAP::Parameter(p));
  }

  return params;
}

void ExtrapModelProvider::buildModels() {
  EXTRAP::CubeFileReader reader;

  auto extrapParams = getParameterList();

  int scalingType = static_cast<int>(ExtraPScalingType::weak);

  std::string cubeFileName("profile");
  // Prefixes can be more complex but should be the same as the parameter names anyway.
  auto keyCont = getKeys(config.params);
  std::vector<std::string> paramPrefixes{std::cbegin(keyCont), std::cend(keyCont)};
  // Actual parameter values: Inner vector is values for the parameter, outer vector corresponds to paramPrefixes
  std::vector<std::vector<int>> paramValues;

  for (const auto &pv : config.params) {
    paramValues.push_back(pv.second);
  }

  // We always access the previous iteration.
  std::string finalDir = config.directory + '/' + 'i' + std::to_string(config.iteration - 1);

  reader.prepareCubeFileReader(scalingType, finalDir, config.prefix, config.postfix, cubeFileName, extrapParams,
                               paramPrefixes, paramValues, config.repetitions);

  int dimensions = extrapParams.size(); // The Extra-P API awaits int instead of size_t
  auto console = spdlog::get("console");
  auto cubeFiles = reader.getFileNames(dimensions);
  auto fns = reader.getFileNames(dimensions);

  const auto &printDbgInfos = [&]() {
    console->debug("Dimension: {}", dimensions);
    for (auto p : extrapParams) {
      console->debug("Param: {}", p.getName());
    }

    console->debug("ParamVals: {}", paramValues.size());
    for (auto p : paramValues) {
      for (auto v : p) {
        console->debug("{}", v);
      }
    }
    std::string dbgOut("Reading cube files:\n");
    for (const auto &f : cubeFiles) {
      dbgOut += "- " + f + "\n";
    }
    console->debug(dbgOut);
  };

  printDbgInfos();

  for (size_t i = 0; i < fns.size(); ++i) {
    //    if (i % configPtr.repetitions == 0) {
    const auto attEpData = [&](auto &cube, auto cnode, auto n) {
      console->debug("Attaching Cube info from file {}", fns.at(i));
      auto ptd = getOrCreateMD<pira::PiraTwoData>(n, ExtrapConnector({}, {}));
      ptd->setExtrapParameters(config.params);
      ptd->addToRuntimeVec(CubeCallgraphBuilder::impl::time(cube, cnode));
    };

    auto &mcgManager = metacg::graph::MCGManager::get();
    CubeCallgraphBuilder::impl::build(std::string(fns.at(i)), mcgManager, attEpData);
    //   }
  }

  for (const auto &n : metacg::pgis::PiraMCGProcessor::get()) {
    console->trace("No PiraTwoData meta data");
    if (n->has<pira::PiraTwoData>()) {
      auto ptd = CubeCallgraphBuilder::impl::get<pira::PiraTwoData>(n);
      const auto la = [&]() {
        std::string s;
        for (auto rtv : ptd->getRuntimeVec()) {
          s += std::to_string(rtv) + " ";
        }
        return s;
      };
      console->debug("ExtrapModelProvider::buildModels: Node {} has {} many runtime values: {}", n->getFunctionName(),
                     ptd->getRuntimeVec().size(), la());
    }
  }

  try {
    console->info("Read cubes with Extra-P library");
    experiment = reader.readCubeFiles(dimensions);
  } catch (std::exception &e) {
    spdlog::get("errconsole")->warn("CubeReader failed with message:\n{}", e.what());
  }

  if (!experiment) {
    spdlog::get("errconsole")->error("No experiment was constructed. Aborting.");
    abort();
  }

  console->info("Reading of experiment CUBEs done.");

  /*
   * Notes
   * - We can have multiple models for one function, as Extra-P models based on the call-path profiles.
   * - We have models for each metric captured in the target application.
   */
  EXTRAP::ModelGenerator *mg = nullptr; // deleted in class destructor
  if (extrapParams.size() == 1) {
    auto smg = new EXTRAP::SingleParameterSimpleModelGenerator();
    smg->setEpsilon(0.05);
    smg->setMaxTermCount(1);
    smg->generateDefaultHypothesisBuildingBlockSet();
    mg = smg;
  } else {
    auto mmg = new EXTRAP::MultiParameterSimpleModelGenerator();
    mg = mmg;
  }

  experiment->addModelGenerator(mg);
  experiment->modelAll(*mg);

  // Get all regions in the experiment
  auto regions = experiment->getRegions();
  // RootCallpath is apparently only executable name.
  auto callPaths = experiment->getAllCallpaths();
  auto metrics = experiment->getMetrics();

  // Retrieve the actual models for the given regions and call paths
  for (const auto &cp : callPaths) {
    for (const auto &m : metrics) {
      if (m->getName() != "time") {
        continue;
      }
      console->debug("Processing for {}", cp->getRegion()->getName());
      auto functionModels = experiment->getModels(*m, *cp);

      for (auto i : functionModels) {
        if (i == nullptr) {
          spdlog::get("errconsole")->warn("Function model is NULL");
          assert(false && "the function model should not be nullptr");
          // What happened if it is indeed nullptr?
        }
        console->debug("{} >>>> {}", cp->getRegion()->getName(), i->getModelFunction()->getAsString(extrapParams));
      }
      auto &elem = models[cp->getRegion()->getName()];
      elem.insert(elem.end(), std::begin(functionModels), std::end(functionModels));
    }
  }

  console->info("Finished model creation.");
}

void ExtrapConnector::modelAggregation(pgis::config::ModelAggregationStrategy modelAggregationStrategy) {
  if (modelAggregationStrategy == pgis::config::ModelAggregationStrategy::Sum) {
    epModelFunction = std::make_unique<SumFunction>(models);
  } else if (modelAggregationStrategy == pgis::config::ModelAggregationStrategy::FirstModel) {
    epModelFunction = std::make_unique<FirstModelFunction>(models);
  } else if (modelAggregationStrategy == pgis::config::ModelAggregationStrategy::Average) {
    epModelFunction = std::make_unique<AvgFunction>(models);
  } else if (modelAggregationStrategy == pgis::config::ModelAggregationStrategy::Maximum) {
    epModelFunction = std::make_unique<MaxFunction>(models);
  }
}
}  // namespace extrapconnection

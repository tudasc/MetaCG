/**
 * File: ExtrapConnection.cpp
 * License: Part of the MetaCG project. Licensed under BSD 3 clause license. See LICENSE.txt file at
 * https://github.com/tudasc/metacg/LICENSE.txt
 */

#include "ExtrapConnection.h"
#include "CubeReader.h"
#include "ErrorCodes.h"

#include "nlohmann/json.hpp"
#include <filesystem>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wextra"
#pragma GCC diagnostic ignored "-Wpedantic"
#pragma GCC diagnostic ignored "-Wunused-variable"
#pragma GCC diagnostic ignored "-Wsign-compare"
#include "EXTRAP_SingleParameterModelGenerator.hpp"
#include "PiraMCGProcessor.h"
#include <EXTRAP_MultiParameterSimpleModelGenerator.hpp>
#include <EXTRAP_SingleParameterSimpleModelGenerator.hpp>
#include <config/PiraIIConfig.h>
#pragma GCC diagnostic pop

namespace extrapconnection {
void printConfig(ExtrapConfig& cfg) {
  auto console = metacg::MCGLogger::instance().getConsole();

  std::string parameterStr;
  for (const auto& p : cfg.params) {
    parameterStr += '(' + p.first + ", [";
    std::vector<int> pVals = p.second;
    for (const auto& v : pVals) {
      parameterStr += std::to_string(v) + ", ";
    }
    parameterStr += "])\n";
  }
  console->info(
      "---- Extra-P Config ----\nBaseDir: {}\nRepetitions: {}\nPrefix: {}\nPostfix: {}\nIterations: {}\nParams: "
      "{}\n---- End Extra-P Config ----",
      cfg.directory, cfg.repetitions, cfg.prefix, cfg.postfix, cfg.iteration, parameterStr);
}

ExtrapConfig getExtrapConfigFromJSON(const std::filesystem::path& filePath) {
  using json = nlohmann::json;

  json j;
  {
    std::ifstream infile(filePath);
    infile >> j;
  }

  auto console = metacg::MCGLogger::instance().getConsole();
  ExtrapConfig cfg;
  for (json::iterator it = j.begin(); it != j.end(); ++it) {
    auto key = it.key();
    console->debug("Iterating for {}", key);

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
        const std::string paramName = '.' + st_it.key();
        std::vector<std::string> paramStrs = st_it.value().get<std::vector<std::string>>();
        std::vector<int> paramValues;
        paramValues.reserve(paramStrs.size());
        std::transform(std::begin(paramStrs), std::end(paramStrs), std::back_inserter(paramValues),
                       [&](std::string& s) {
                         console->debug("Transforming {}", s);
                         return std::stoi(s);
                       });
        cfg.params.emplace_back(paramName, paramValues);
      }
    } else {
      metacg::MCGLogger::instance().getErrConsole()->warn("This should not happen! Unkown json identifier found.");
    }
    std::reverse(std::begin(cfg.params), std::end(cfg.params));
    for (auto& p : cfg.params) {
      console->debug("{}", p.first);
    }
  }

  // XXX How to have this neat and tidy in a single statement?
  console->info("Parsed File. Resulting Config:");
  printConfig(cfg);

  return cfg;
}

const auto getKeys = [](const auto c) {
  std::vector<typename decltype(c)::value_type::first_type> keys;
  for (const auto& e : c) {
    if (std::find(std::begin(keys), std::end(keys), e.first) == std::end(keys)) {
      keys.push_back(e.first);
    }
  }
  return keys;
};

std::vector<EXTRAP::Parameter> ExtrapModelProvider::getParameterList() {
  std::vector<EXTRAP::Parameter> params;
  params.reserve(config.params.size());

  for (const auto& p : getKeys(config.params)) {
    params.emplace_back(p);
  }

  return params;
}

void ExtrapModelProvider::buildModels() {
  EXTRAP::CubeFileReader reader;

  auto extrapParams = getParameterList();

  const int scalingType = static_cast<int>(ExtraPScalingType::weak);

  std::string cubeFileName("profile");
  // Prefixes can be more complex but should be the same as the parameter names anyway.
  auto keyCont = getKeys(config.params);
  std::vector<std::string> paramPrefixes{std::cbegin(keyCont), std::cend(keyCont)};
  // Actual parameter values: Inner vector is values for the parameter, outer vector corresponds to paramPrefixes
  std::vector<std::vector<int>> paramValues;

  paramValues.reserve(config.params.size());
  for (const auto& pv : config.params) {
    paramValues.push_back(pv.second);
  }

  auto console = metacg::MCGLogger::instance().getConsole();
  auto errConsole = metacg::MCGLogger::instance().getErrConsole();

  // We always access the previous iteration.
  const std::string finalDir = config.directory + '/' + 'i' + std::to_string(config.iteration - 1);

  reader.prepareCubeFileReader(scalingType, finalDir, config.prefix, config.postfix, cubeFileName, extrapParams,
                               paramPrefixes, paramValues, config.repetitions);

  const size_t dimensions = extrapParams.size();  // The Extra-P API awaits int instead of size_t
  auto fns = reader.getFileNames(dimensions);

  const auto& printDbgInfos = [&]() {
    console->debug("Dimension: {}", dimensions);
    for (const auto& p : extrapParams) {
      console->debug("Param: {}", p.getName());
    }

    console->debug("ParamVals: {}", paramValues.size());
    for (const auto& p : paramValues) {
      for (auto v : p) {
        console->debug("{}", v);
      }
    }
    std::string dbgOut("Reading cube files:\n");
    for (const auto& f : fns) {
      dbgOut += "- " + f + "\n";
    }
    console->debug(dbgOut);
  };

  // XXX why is that here?
  printDbgInfos();

  for (auto& fn : fns) {
    const auto attEpData = [&](auto& cube, auto cnode, auto n, [[maybe_unused]] auto pnode, [[maybe_unused]] auto pn) {
      console->debug("Attaching Cube info from file {}", fn);
      auto ptd = n->getOrCreateMD<pira::PiraTwoData>(ExtrapConnector({}, {}));
      ptd->setExtrapParameters(config.params);
      ptd->addToRuntimeVec(metacg::pgis::impl::time(cube, cnode));
    };

    auto& mcgManager = metacg::graph::MCGManager::get();
    metacg::pgis::impl::build(std::string(fn), mcgManager, attEpData);
    //   }
  }

  for (const auto& elem : metacg::pgis::PiraMCGProcessor::get()) {
    const auto& n = elem.second.get();
    console->trace("No PiraTwoData meta data");
    if (n->has<pira::PiraTwoData>()) {
      auto ptd = metacg::pgis::impl::get<pira::PiraTwoData>(n);
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
  } catch (std::exception& e) {
    errConsole->warn("CubeReader failed with message:\n{}", e.what());
  }

  if (!experiment) {
    errConsole->error("No experiment was constructed. Aborting.");
    abort();
  }

  console->info("Reading of experiment CUBEs done.");

  /*
   * Notes
   * - We can have multiple models for one function, as Extra-P models based on the call-path profiles.
   * - We have models for each metric captured in the target application.
   */
  EXTRAP::ModelGenerator* mg = nullptr;  // deleted in class destructor
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
  for (const auto& cp : callPaths) {
    for (const auto& m : metrics) {
      if (m->getName() != "time") {
        continue;
      }
      console->debug("Processing for {}", cp->getRegion()->getName());
      auto functionModels = experiment->getModels(*m, *cp);

      for (auto i : functionModels) {
        if (i == nullptr) {
          errConsole->warn("Function model is NULL");
          assert(false && "the function model should not be nullptr");
          // What happened if it is indeed nullptr?
        }
        console->debug("{} >>>> {}", cp->getRegion()->getName(), i->getModelFunction()->getAsString(extrapParams));
      }
      auto& elem = models[cp->getRegion()->getName()];
      elem.insert(elem.end(), std::begin(functionModels), std::end(functionModels));
    }
  }

  console->info("Finished model creation.");
}

void ExtrapConnector::modelAggregation(metacg::pgis::config::ModelAggregationStrategy modelAggregationStrategy) {
  if (modelAggregationStrategy == metacg::pgis::config::ModelAggregationStrategy::Sum) {
    epModelFunction = std::make_unique<SumFunction>(models);
  } else if (modelAggregationStrategy == metacg::pgis::config::ModelAggregationStrategy::FirstModel) {
    epModelFunction = std::make_unique<FirstModelFunction>(models);
  } else if (modelAggregationStrategy == metacg::pgis::config::ModelAggregationStrategy::Average) {
    epModelFunction = std::make_unique<AvgFunction>(models);
  } else if (modelAggregationStrategy == metacg::pgis::config::ModelAggregationStrategy::Maximum) {
    epModelFunction = std::make_unique<MaxFunction>(models);
  }
}
}  // namespace extrapconnection

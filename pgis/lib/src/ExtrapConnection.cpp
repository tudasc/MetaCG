//
// Created by jp on 04.04.19.
//

#include "ExtrapConnection.h"

#include "nlohmann/json.hpp"
#include "spdlog/spdlog.h"

#include "EXTRAP_SingleParameterModelGenerator.hpp"
#include <EXTRAP_MultiParameterSimpleModelGenerator.hpp>
#include <EXTRAP_SingleParameterSimpleModelGenerator.hpp>

namespace extrapconnection {

void printConfig(ExtrapConfig &cfg) {
  std::cout << "ExtrapConfig\nBaseDir: " << cfg.directory << "\nRepetitions: " << cfg.repetitions
            << "\nPrefix: " << cfg.prefix << "\nPostfix: " << cfg.postfix << "\nIterations: " << cfg.iteration
            << "\nParams:\n";
  for (const auto &p : cfg.params) {
    std::cout << "( " << p.first << ", [";
    std::vector<int> pVals = p.second;
    for (const auto &v : pVals) {
      std::cout << v << ", ";
    }
    std::cout << "])\n";
  }
  std::cout << " ---- " << std::endl;
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
    std::cout << "Iterating for " << key << std::endl;

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
      // TODO double check if this is the correct sub-tree
      for (json::iterator st_it = subTree.begin(); st_it != subTree.end(); ++st_it) {
        // This case should only be hit, when actually a PARAMETER!
        std::string paramName = '.' + st_it.key();
        std::vector<std::string> paramStrs = st_it.value().get<std::vector<std::string>>();
        std::vector<int> paramValues;
        paramValues.reserve(paramStrs.size());
        std::transform(std::begin(paramStrs), std::end(paramStrs), std::back_inserter(paramValues), [](std::string &s) {
          std::cout << "Transforming " << s << std::endl;
          return std::stoi(s);
        });
        cfg.params.emplace_back(std::make_pair(paramName, paramValues));
      }
    } else {
      std::cerr << "This should not happen\n" << std::endl;
    }
    std::reverse(std::begin(cfg.params), std::end(cfg.params));
    for (auto &p : cfg.params) {
      std::cout << "p.first: " << p.first << std::endl;
    }
  }

  std::cout << "Parsed File. Resulting Config:\n";
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

  for (const auto p : getKeys(config.params)) {
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

  for (const auto pv : config.params) {
    paramValues.push_back(pv.second);
  }

  for (auto pp : paramPrefixes) {
    std::cout << pp << std::endl;
  }

  // We always access the previous iteration.
  std::string finalDir = config.directory + '/' + 'i' + std::to_string(config.iteration - 1);

  reader.prepareCubeFileReader(scalingType, finalDir, config.prefix, config.postfix, cubeFileName, extrapParams,
                               paramPrefixes, paramValues, config.repetitions);

  int dimensions = extrapParams.size();
  std::cout << "Dimension: " << dimensions << std::endl;
  for (auto p : extrapParams) {
    std::cout << p.getName() << std::endl;
  }

  std::cout << "ParamVals: " << paramValues.size() << std::endl;
  for (auto p : paramValues) {
    for (auto v : p) {
      std::cout << v << std::endl;
    }
  }

  auto cubeFiles = reader.getFileNames(dimensions);
  std::string dbgOut("Reading cube files:\n");
  for (const auto f : cubeFiles) {
    dbgOut += "- " + f + "\n";
  }
  spdlog::get("console")->debug(dbgOut);

  std::cout << "Read cube files for experiment data." << std::endl;
  try {
    experiment = reader.readCubeFiles(dimensions);
  } catch (std::exception &e) {
    std::cerr << e.what() << std::endl;
  }

  if (!experiment) {
    std::cerr << "Reading of cube files went wrong. Aborting" << std::endl;
    abort();
  }

  std::cout << "Read cube files for experiment data DONE." << std::endl;

  /*
   * Notes
   * - We can have multiple models for one function, as Extra-P models based on the call-path profiles.
   * - We have models for each metric captured in the target application.
   * - We need the call paths to retrieve the model we want, i.e., need to construct the call path.
   */

  // FIXME: Delete object / prevent resource leakage
  EXTRAP::ModelGenerator *mg = nullptr;
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

  if (!mg) {
    std::cerr << "Creation of Model Generator failed." << std::endl;
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
      std::cout << "Processing for " << cp->getRegion()->getName() << std::endl;
      // Currently only one model is generated at a time. // How to handle multiple?
      auto functionModels = experiment->getModels(*m, *cp);

      for (auto i : functionModels) {
        if (!i) {
          std::cerr << "Function model is NULL" << std::endl;
          auto tf = i->getModelFunction();
          if (!tf) {
            std::cerr << "Modelfunction is NULL" << std::endl;
          }
        }
        std::cout << i->getModelFunction()->getAsString(extrapParams) << std::endl;
      }
      auto &elem = models[cp->getRegion()->getName()];
      elem.insert(elem.end(), std::begin(functionModels), std::end(functionModels));
    }
  }

  std::cout << "Finished model creation." << std::endl;
}

}  // namespace extrapconnection

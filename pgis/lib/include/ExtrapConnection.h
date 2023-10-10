/**
 * File: ExtrapConnection.h
 * License: Part of the metacg project. Licensed under BSD 3 clause license. See LICENSE.txt file at
 * https://github.com/tudasc/metacg/LICENSE.txt
 */

//
// Created by jp on 01.04.19.
//

#ifndef PGOE_EXTRAPCONNECTION_H
#define PGOE_EXTRAPCONNECTION_H

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wextra"
#pragma GCC diagnostic ignored "-Wpedantic"
#pragma GCC diagnostic ignored "-Wunused-variable"
#pragma GCC diagnostic ignored "-Wsign-compare"
#include "EXTRAP_CubeFileReader.hpp"
#include "EXTRAP_Experiment.hpp"
#pragma GCC diagnostic pop

#include "spdlog/spdlog.h"

#include "cxxabi.h"

#include <filesystem>
#include <map>
#include <numeric>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "ExtrapAggregatedFunctions.h"
#include "config/PiraIIConfig.h"

namespace cube {
class Cnode;
}

namespace extrapconnection {

enum class ExtraPScalingType { weak = 1, strong = 2 };

struct ExtrapConfig {
  std::string directory;
  std::string prefix;
  std::string postfix;
  // std::map<std::string, std::vector<int>> params;
  std::vector<std::pair<std::string, std::vector<int>>> params;
  int repetitions;
  int iteration;
};

class ExtrapExtrapolator {
 public:
  explicit ExtrapExtrapolator(std::vector<std::pair<std::string, std::vector<int>>> values)
      : values(std::move(values)) {}

  auto getValues() { return values; }

  auto getExtrapolationValue(double alpha = 1.0) {
    std::vector<std::pair<std::string, double>> result;
    // We need to compute extrapolation values for all params
    for (const auto &param_pair : values) {
      std::vector<double> steps;
      auto it1 = param_pair.second.begin();
      auto it2 = param_pair.second.begin();
      std::advance(it1, 1);
      for (; it1 != param_pair.second.end(); ++it1, ++it2) {
        steps.push_back(alpha * (static_cast<double>((*it1)) - (*it2)));
      }
      auto sum = std::accumulate(std::begin(steps), std::end(steps), .0);
      // basic linear interpolation based on input values
      auto interPolValue = sum / steps.size();
      auto lastVal = *(it2);
      result.emplace_back(std::make_pair(param_pair.first, lastVal + interPolValue));
      // result.emplace_back(std::make_pair(param_pair.first, 4 * param_pair.second.back()));
    }

    return result;
  }

 private:
  // static ExtrapExtrapolator *instance;
  std::vector<std::pair<std::string, std::vector<int>>> values;
};

/* Utility functions to read in configuration and print it */
ExtrapConfig getExtrapConfigFromJSON(const std::filesystem::path &filePath);

void printConfig(ExtrapConfig &cfg);

inline std::string demangle(std::string input) {
  char *res;
  int st;
  res = abi::__cxa_demangle(input.c_str(), nullptr, nullptr, &st);
  if (st < 0) {
    return input;
  }
  std::string demangledNanme(res);
  free(res);
  return demangledNanme;
}

class ExtrapConnector {
 public:
  explicit ExtrapConnector(std::vector<EXTRAP::Model *> models, EXTRAP::ParameterList parameterList)
      : models(models), paramList(parameterList), epolator(ExtrapExtrapolator({})) {
    spdlog::get("console")->trace("ExtrapConnector: explicit ctor: models {}", models.size());
  }

  ~ExtrapConnector() = default;

  ExtrapConnector(const ExtrapConnector &other)
      : epModelFunction(std::make_unique<EXTRAP::Function>(*other.epModelFunction)),
        models(other.models),
        paramList(other.paramList),
        epolator(other.epolator) {
    spdlog::get("console")->trace("ExtrapConnector: copy ctor\nother.models: {}\nthis.models: {}", other.models.size(),
                                  this->models.size());
  }

  ExtrapConnector &operator=(const ExtrapConnector &other) {
    this->epModelFunction = std::make_unique<EXTRAP::Function>(*other.epModelFunction);
    this->models = other.models;
    this->paramList = other.paramList;
    this->epolator = other.epolator;
    spdlog::get("console")->trace("ExtrapConnector: copy assignment operator\nother.models: {}\nthis.models: {}",
                                  other.models.size(), this->models.size());
    return *this;
  }

  /* Test whether models were generated and set */
  bool hasModels() const { return models.size() > 0; }
  /* Return number of associated models */
  std::size_t modelCount() const { return models.size(); }
  /* Check whether specific model hast been set */
  bool isModelSet() const { return epModelFunction != nullptr; }
  std::string getModelStrings() const {
    std::stringstream ss;
    for (auto f : models) {
      ss << f->getModelFunction()->getAsString(paramList) << "; ";
    }
    std::string s = ss.str();
    if (!s.empty()) {
      s.pop_back();
      s.pop_back();
    }
    return s;
  }

  /* Get the specific model set */
  auto &getEPModelFunction() const { return epModelFunction; }
  std::string getEPModelFunctionAsString() const {
    if (isModelSet()) {
      return epModelFunction->getAsString(paramList);
    } else {
      return "not set!";
    }
  }
  auto getParamList() const { return this->paramList; }
  auto getEpolator() const { return epolator; }

  /* Set the specific model */
  void setEpolator(ExtrapExtrapolator e) { epolator = e; }

  /**
   * Aggregates (or selects) a single model (function) from the set of available models.
   * Call only once!
   * @param modelAggregationStrategy Aggregration/Selection strategy
   */
  void modelAggregation(pgis::config::ModelAggregationStrategy modelAggregationStrategy);

 private:
  std::unique_ptr<EXTRAP::Function> epModelFunction = nullptr;

  std::vector<EXTRAP::Model *> models;
  EXTRAP::ParameterList paramList;

  ExtrapExtrapolator epolator;
};

class ExtrapModelProvider {
 public:
  explicit ExtrapModelProvider(ExtrapConfig config) : config(std::move(config)), experiment(nullptr) {}
  ~ExtrapModelProvider() {
    if (experiment) {
      auto generators = experiment->getModelGenerators();
      for (const auto &gen : generators) {
        delete gen;  // clean up what we allocated
      }
    }
  }
  // Needs to be defined in header, so deduction works correctly in usages.
  auto getModelFor(const std::string &functionName) {
    if (models.empty()) {
      buildModels();
    }

    // PGIS uses mangled names, Extra-P apparently demangled names.
    // XXX This may have changed in libcube 4.5? Double Check!
    auto demangledName = demangle(functionName);

    auto paramList = getParameterList();

    auto m = models[functionName];
    spdlog::get("console")->debug(
        "ModelProvider:getModelFor {}: {}\nUsing mangled name: {}", demangledName,
        m.size() > 0 ? m.front()->getModelFunction()->getAsString(getParameterList()) : " NONE ", functionName);
    return ExtrapConnector(m, paramList);
  }

  std::vector<EXTRAP::Parameter> getParameterList();

  std::vector<double> getConfigValues(const std::string &key) {
    std::vector<double> vals;
    vals.reserve(config.params.size());
    // FIXME for now we assume only a single parameter!!
    for (const auto &p : config.params) {
      if (key == p.first) {
        for (const auto v : p.second) {
          vals.emplace_back(v);
        }
      }
    }
    return vals;
  }

  auto getConfigValues() const { return config.params; }

  void buildModels();

 private:
  ExtrapConfig config;
  // Hold mapping mangled_name -> Models
  std::unordered_map<std::string, std::vector<EXTRAP::Model *>> models;
  EXTRAP::Experiment *experiment;
};

}  // namespace extrapconnection

#endif  // PGOE_EXTRAPCONNECTION_H

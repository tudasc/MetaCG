/**
 * File: ExtrapConnection.h
 * License: Part of the MetaCG project. Licensed under BSD 3 clause license. See LICENSE.txt file at https://github.com/tudasc/metacg/LICENSE.txt
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

#include <map>
#include <numeric>
#include <string>
#include <unordered_map>
#include <vector>

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
  ExtrapExtrapolator(std::vector<std::pair<std::string, std::vector<int>>> values) : values(values) {}

  auto getValues() { return values; }

  auto getExtrapolationValue(double alpha = 1.0) {
    std::vector<std::pair<std::string, double>> result;
    // We need to compute extrapolation values for all params
    for (const auto param_pair : values) {
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
ExtrapConfig getExtrapConfigFromJSON(std::string filePath);

void printConfig(ExtrapConfig &cfg);

inline std::string demangle(std::string &input) {
  char *res;
  int st;
  res = abi::__cxa_demangle(input.c_str(), 0, 0, &st);
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

  ExtrapConnector(const ExtrapConnector &other)
      : epModel(other.epModel), models(other.models), paramList(other.paramList), epolator(other.epolator) {
    spdlog::get("console")->trace("ExtrapConnector: copy ctor\nother.models: {}\nthis.models: {}", other.models.size(),
                                  this->models.size());
  }

  /* Test whether models were generated and set */
  bool hasModels() const { return models.size() > 0; }
  /* Check whether specific model hast been set */
  bool isModelSet() const { return epModel != nullptr; }

  /* Get the specific model set */
  auto getEPModelFunction() const { return epModel->getModelFunction(); }
  auto getParamList() const { return this->paramList; }
  auto getEpolator() const { return epolator; }

  /* Set the specific model */
  void setEPMetric(EXTRAP::Model *m) { epModel = m; }
  void setEpolator(ExtrapExtrapolator e) { epolator = e; }

  void useFirstModel() { epModel = models.front(); }

  inline void printModels(std::ostream &os = std::cout) const {
    os << epModel->getModelFunction() << " : ";
    for (auto f : models) {
      os << f->getModelFunction() << std::endl;
    }
  }

 private:
  EXTRAP::Model *epModel = nullptr;
  std::vector<EXTRAP::Model *> models;
  EXTRAP::ParameterList paramList;

  ExtrapExtrapolator epolator;
};

class ExtrapModelProvider {
 public:
  explicit ExtrapModelProvider(ExtrapConfig config) : config(config) {}

  // Needs to be defined in header, so deduction works correctly in usages.
  auto getModelFor(std::string functionName) {
    if (models.empty()) {
      buildModels();
    }

    // PGIS uses mangled names, Extra-P apparently demangled names.
    auto demangledName = demangle(functionName);
    // std::cout << "Mangled: " << functionName << "\nDemangl: " << demangledName << std::endl;

    auto paramList = getParameterList();

    auto m = models[demangledName];
    spdlog::get("console")->debug(
        "ModelProvider:getModelFor {}: {}", demangledName,
        m.size() > 0 ? m.front()->getModelFunction()->getAsString(getParameterList()) : " NONE ");
    return ExtrapConnector(m, paramList);
  }

  std::vector<EXTRAP::Parameter> getParameterList();

  std::vector<double> getConfigValues(std::string key) {
    std::vector<double> vals;
    vals.reserve(config.params.size());
    // FIXME for now we assume only a single parameter!!
    for (const auto p : config.params) {
      if (key == p.first) {
        for (const auto v : p.second) {
          vals.emplace_back(v);
        }
      }
    }
    return vals;
  }

  auto getConfigValues() { return config.params; }

  void buildModels();

 private:
  ExtrapConfig config;
  // Hold mapping mangled_name -> Models
  std::unordered_map<std::string, std::vector<EXTRAP::Model *>> models;
  EXTRAP::Experiment *experiment;
};

}  // namespace extrapconnection

#endif  // PGOE_EXTRAPCONNECTION_H

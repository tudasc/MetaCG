/**
 * File: ParameterConfig.cpp
 * License: Part of the MetaCG project. Licensed under BSD 3 clause license. See LICENSE.txt file at
 * https://github.com/tudasc/metacg/LICENSE.txt
 */

#include "config/ParameterConfig.h"

using namespace pgis::config;

ParameterConfig::ParameterConfig() {
  auto &gConfig = pgis::config::GlobalConfig::get();
  auto path = gConfig.getAs<std::string>(pgis::options::parameterFileConfig.cliName);

  // read file
  std::ifstream file(path);
  nlohmann::json json;

  try {
    file >> json;

    // parsing of configuration chapters
    // =================================

    // Pira II configuration
    const std::string piraIIName = "Modeling";
    if (json.contains(piraIIName)) {
      this->piraIIconfig = std::make_unique<PiraIIConfig>();
      json.at(piraIIName).get_to<PiraIIConfig>(*this->piraIIconfig);
    }

    // Pira LIDe configuration
    const std::string lideName = "LIDe";
    if (json.contains(lideName)) {
      this->liConfig = std::make_unique<LoadImbalance::LIConfig>();
      json.at(lideName).get_to<LoadImbalance::LIConfig>(*this->liConfig);
    }
  } catch (nlohmann::json::exception &e) {
    spdlog::get("errconsole")->error("Unable to parse parameter configuration file: {}", e.what());
    exit(EXIT_FAILURE);
  }
}

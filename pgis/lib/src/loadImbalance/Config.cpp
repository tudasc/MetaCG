/**
 * File: Config.cpp
 * License: Part of the MetaCG project. Licensed under BSD 3 clause license. See LICENSE.txt file at https://github.com/tudasc/metacg/LICENSE.txt
 */

#include "loadImbalance/Config.h"

using namespace LoadImbalance;
using namespace nlohmann;

Config Config::generateFromJSON(std::string configPath) {
  // read file
  std::ifstream i(configPath);
  json j;
  i >> j;

  // parse json
  Config c = j.get<LoadImbalance::Config>();

  return c;
}

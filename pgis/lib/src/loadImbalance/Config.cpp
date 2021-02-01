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
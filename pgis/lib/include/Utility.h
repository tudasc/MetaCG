/**
 * File: Utility.h
 * License: Part of the MetaCG project. Licensed under BSD 3 clause license. See LICENSE.txt file at https://github.com/tudasc/metacg/LICENSE.txt
 */

#ifndef PGIS_UTILITY_H
#define PGIS_UTILITY_H

#include "spdlog/spdlog.h"

#include "unistd.h"

#include <cerrno>
#include <string>
#include <cstring>

namespace {
inline std::string getHostname() {
  char *cName = (char *)malloc(255 * sizeof(char));
  if (!gethostname(cName, 255)) {
    spdlog::get("errconsole")->error("Unable to determine hostname");
  } else {
    spdlog::get("console")->debug("Running on host: {}", cName);
  }
  std::string name(cName);
  free(cName);
  return name;
}
}  // namespace

#endif

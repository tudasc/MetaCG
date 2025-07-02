/**
 * File: MCGReader.cpp
 * License: Part of the MetaCG project. Licensed under BSD 3 clause license. See LICENSE.txt file at
 * https://github.com/tudasc/metacg/LICENSE.txt
 *
 */

#include "io/MCGReader.h"
#include "LoggerUtil.h"
#include "io/VersionFourMCGReader.h"
#include "io/VersionTwoMCGReader.h"

namespace metacg::io {

std::unique_ptr<MetaCGReader> createReader(ReaderSource& src) {
  auto versionStr = src.getFormatVersion();
  if (versionStr == "2" || versionStr == "2.0") {
    return std::make_unique<VersionTwoMetaCGReader>(src);
  } else if (versionStr == "3" || versionStr == "3.0") {
    return std::make_unique<VersionThreeMetaCGReader>(src);
  } else {
    metacg::MCGLogger::instance().getErrConsole()->error("Cannot create reader: format version '{}' is not suppported.",
                                                         versionStr);
    return {};
  }
}

}  // namespace metacg::io

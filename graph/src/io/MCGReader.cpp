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

std::unique_ptr<MCGReader> createReader(ReaderSource& src) {
  auto versionStr = src.getFormatVersion();
  if (versionStr == "2" || versionStr == "2.0") {
    return std::make_unique<VersionTwoMCGReader>(src);
  }
  if (versionStr == "3" || versionStr == "3.0") {
    metacg::MCGLogger::logError(
        "Format version 3 is no longer supported. Please use the version 4 reader instead. Old version 3 files can be "
        "converted using the 'cgconvert' tool included in the MetaCG v0.8 release.");
    return {};
  } else if (versionStr == "4" || versionStr == "4.0") {
    return std::make_unique<VersionFourMCGReader>(src);
  } else {
    metacg::MCGLogger::logError("Cannot create reader: format version '{}' is not suppported.", versionStr);
    return {};
  }
}

}  // namespace metacg::io

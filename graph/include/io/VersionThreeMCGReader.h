/**
 * File: VersionThreeMCGReader.h
 * License: Part of the MetaCG project. Licensed under BSD 3 clause license. See LICENSE.txt file at
 * https://github.com/tudasc/metacg/LICENSE.txt
 */

#ifndef METACG_VERSIONTHREEMCGREADER_H
#define METACG_VERSIONTHREEMCGREADER_H

#include "MCGReader.h"

namespace metacg::io {

class VersionThreeMetaCGReader : public metacg::io::MetaCGReader {
 public:
  explicit VersionThreeMetaCGReader(metacg::io::ReaderSource& source) : MetaCGReader(source) {}
  std::unique_ptr<Callgraph> read() override;
  static void convertFromDebug(json& j);
  static bool isV3DebugFormat(const json& j);
};

}  // end namespace metacg::io

#endif  // METACG_VERSIONTHREEMCGREADER_H
/**
 * File: VersionFourMCGReader.h
 * License: Part of the MetaCG project. Licensed under BSD 3 clause license. See LICENSE.txt file at
 * https://github.com/tudasc/metacg/LICENSE.txt
 */

#ifndef METACG_VERSIONFOURMCGREADER_H
#define METACG_VERSIONFOURMCGREADER_H

#include "MCGReader.h"

namespace metacg::io {

class VersionFourMCGReader : public metacg::io::MCGReader {
 public:
  explicit VersionFourMCGReader(metacg::io::ReaderSource& source) : MCGReader(source) {}
  std::unique_ptr<Callgraph> read() override;
};

}  // end namespace metacg::io

#endif  // METACG_VERSIONFOURMCGREADER_H
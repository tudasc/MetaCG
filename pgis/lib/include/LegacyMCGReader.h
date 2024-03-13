/**
 * File: LegacyMCGReader.h
 * License: Part of the metacg project. Licensed under BSD 3 clause license. See LICENSE.txt file at
 * https://github.com/tudasc/metacg/LICENSE.txt
 */

#ifndef METACG_PGIS_INCLUDE_LEGACYMCGREADER_H
#define METACG_PGIS_INCLUDE_LEGACYMCGREADER_H

#include "MCGReader.h"

namespace metacg::pgis::io {

/**
 * Class to read metacg files in file format v1.0.
 * The file format is also typically referred to as IPCG files.
 */
class VersionOneMetaCGReader : public metacg::io::MetaCGReader {
 public:
  explicit VersionOneMetaCGReader(metacg::io::ReaderSource &source) : MetaCGReader(source) {}
  void read(metacg::graph::MCGManager &cgManager) override;

 private:
  void addNumStmts(metacg::graph::MCGManager &cgm);
};
}  // namespace metacg::pgis::io

#endif

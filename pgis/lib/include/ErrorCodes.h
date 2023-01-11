//
// Created by jp on 16.09.22.
//

#ifndef METACG_ERRORCODES_H
#define METACG_ERRORCODES_H

namespace pgis {

/**
 * Holds return codes for the PGIS Analyzer
 */
enum ErrorCode : int {
  SUCCESS = 0,
  NoGraphConstructed,
  NoMainFunctionFound,
  ErroneousHeuristicsConfiguration,
  UnknownFileFormat,
  TooFewProgramArguments = 1024
};
}

#endif  // METACG_ERRORCODES_H

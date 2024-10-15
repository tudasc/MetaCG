//
// Created by jp on 16.09.22.
//

#ifndef METACG_ERRORCODES_H
#define METACG_ERRORCODES_H

#include <string_view>

namespace metacg::pgis {

/**
 * Holds return codes for the PGIS Analyzer
 */
enum ErrorCode : int {
  SUCCESS = 0,
  NoGraphConstructed,
  NoMainFunctionFound,
  ErroneousHeuristicsConfiguration,
  UnknownFileFormat,
  ErroneousOverheadConfiguration,
  CouldNotGetCWD,
  FileDoesNotExist,
  TooFewProgramArguments = 1024
};

inline std::string_view to_string(ErrorCode err) {
  switch (err) {
    case pgis::ErrorCode::SUCCESS:
      return "Success";
    case pgis::ErrorCode::NoGraphConstructed:
      return "No graph constructed error";
    case pgis::ErrorCode::NoMainFunctionFound:
      return "No main function found error";
    case pgis::ErrorCode::ErroneousHeuristicsConfiguration:
      return "Erroneous heuristics configuration errro";
    case pgis::ErrorCode::UnknownFileFormat:
      return "Unknown file format error";
    case pgis::ErrorCode::ErroneousOverheadConfiguration:
      return "Erroneous Overhead Configuration error";
    case pgis::ErrorCode::CouldNotGetCWD:
      return "Could not get current working directory error";
    case pgis::ErrorCode::FileDoesNotExist:
      return "File does not exist error";
    case pgis::ErrorCode::TooFewProgramArguments:
      return "Too few program arguments error";
  }
  return "Some unknown error";
}
}  // namespace metacg::pgis

#endif  // METACG_ERRORCODES_H

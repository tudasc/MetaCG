/**
 * File: LoggerUtil.h
 * License: Part of the metacg project. Licensed under BSD 3 clause license. See LICENSE.txt file at https://github.com/tudasc/metacg/LICENSE.txt
 */

#ifndef PGIS_UNITTEST_LOGGERUTIL_H
#define PGIS_UNITTEST_LOGGERUTIL_H

#include "spdlog/sinks/stdout_color_sinks.h"
#include "spdlog/spdlog.h"

namespace loggerutil {
/**
  * Enables output of logged errors.
  *
  * Typically not needed to call explicitly, use the convenience macro instead.
  */
inline void enableErrors() {
  spdlog::get("console")->set_level(spdlog::level::err);
  spdlog::get("errconsole")->set_level(spdlog::level::err);
}
/** 
 * Diables output of logged errors.
 *
 * Typically not needed to call explicitly, use the convenience macro instead.
 */
inline void disableErrors() {
  spdlog::get("console")->set_level(spdlog::level::off);
  spdlog::get("errconsole")->set_level(spdlog::level::off);
}

/**
 * Returns initialized logger object with output disabled.
 */
inline void getLogger() {
  auto c = spdlog::get("console");
  if (!c) {
    c = spdlog::stdout_color_mt("console");
  }
  auto e = spdlog::get("errconsole");
  if (!e) {
    e = spdlog::stderr_color_mt("errconsole");
  }
  disableErrors();
}

/**
 * Enables error outputs in logger.
 * 
 * This is needed by some of the death tests to succeed.
 * Typically not needed to call explicitly, use the convenience macro instead.
 */
struct ErrorOutEnabler {
  ErrorOutEnabler() {
    enableErrors();
  }
  ~ErrorOutEnabler() {
    disableErrors();
  }
};

}  // namespace loggerutil

/**
 * Convenience macro to enable error output for a single scope.
 */
#define LOGGERUTIL_ENABLE_ERRORS_LOCAL loggerutil::ErrorOutEnabler __eoe;

#endif

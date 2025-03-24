/**
 * File: LoggerUtil.h
 * License: Part of the MetaCG project. Licensed under BSD 3 clause license. See LICENSE.txt file at
 * https://github.com/tudasc/metacg/LICENSE.txt
 */
#ifndef METACG_GRAPH_LOGGERUTIL_H
#define METACG_GRAPH_LOGGERUTIL_H

#include "spdlog/sinks/stdout_color_sinks.h"
#include "spdlog/spdlog.h"

namespace metacg {
/**
 * Wrapper to obtain the MetaCG-wide spdlog console/errconsole loggers.
 */
class MCGLogger {
 public:
  static MCGLogger& instance() {
    static MCGLogger instance;
    return instance;
  }

  /**
   * Get non-owning raw pointer to underlying spdlog logger
   * @return
   */
  inline auto getConsole() { return console.get(); }

  /**
   * Get non-owning raw pointer to underlying spdlog logger
   * @return
   */
  inline auto getErrConsole() { return errconsole.get(); }

 private:
  MCGLogger() : console(spdlog::stdout_color_mt("console")), errconsole(spdlog::stderr_color_mt("errconsole")) {}

  std::shared_ptr<spdlog::logger> console;
  std::shared_ptr<spdlog::logger> errconsole;
};

namespace loggerutil {
/**
 * Enables output of logged errors.
 *
 * Typically not needed to call explicitly, use the convenience macro instead.
 */
inline void enableErrors() {
  MCGLogger::instance().getConsole()->set_level(spdlog::level::err);
  MCGLogger::instance().getErrConsole()->set_level(spdlog::level::err);
}
/**
 * Diables output of logged errors.
 *
 * Typically not needed to call explicitly, use the convenience macro instead.
 */
inline void disableErrors() {
  MCGLogger::instance().getConsole()->set_level(spdlog::level::off);
  MCGLogger::instance().getErrConsole()->set_level(spdlog::level::off);
}

/**
 * Returns initialized logger object with output disabled.
 */
inline void getLogger() {
  auto console = MCGLogger::instance().getConsole();
  auto errConsole = MCGLogger::instance().getErrConsole();
  console->set_level(console->level());
  errConsole->set_level(errConsole->level());
  disableErrors();
}

/**
 * Enables error outputs in logger.
 *
 * This is needed by some of the death tests to succeed.
 * Typically not needed to call explicitly, use the convenience macro instead.
 */
struct ErrorOutEnabler {
  ErrorOutEnabler() { enableErrors(); }
  ~ErrorOutEnabler() { disableErrors(); }
};

}  // namespace loggerutil

}  // namespace metacg

/**
 * Convenience macro to enable error output for a single scope.
 */
#define LOGGERUTIL_ENABLE_ERRORS_LOCAL metacg::loggerutil::ErrorOutEnabler __eoe;

#endif
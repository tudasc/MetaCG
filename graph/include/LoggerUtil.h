/**
 * File: LoggerUtil.h
 * License: Part of the MetaCG project. Licensed under BSD 3 clause license. See LICENSE.txt file at
 * https://github.com/tudasc/metacg/LICENSE.txt
 */
#ifndef METACG_GRAPH_LOGGERUTIL_H
#define METACG_GRAPH_LOGGERUTIL_H

#include "spdlog/fmt/bundled/core.h"
#include "spdlog/sinks/stdout_color_sinks.h"
#include "spdlog/spdlog.h"
#include <unordered_set>
#include <iostream>

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

  enum class LogType { DEFAULT, UNIQUE };
  enum class Output { StdConsole, ErrConsole };

  /**
   * Formats and prints the message as log-level info
   *
   * @tparam lt pass UNIQUE if this should be printed only once
   * @tparam outPutType pass ERROR if this should be printed on the error console
   * @tparam MSG_t inferred from msg
   * @tparam Args inferred from args
   * @param msg the msg to be formatted and printed
   * @param args the arguments to format the message with
   */
  template <LogType lt = LogType::DEFAULT, Output outPutType = Output::StdConsole, typename MSG_t, typename... Args>
  void info(const MSG_t msg, Args&&... args) {
    const std::string& formattedMessage = fmt::vformat(std::string_view(msg), fmt::make_format_args(args...));
    if (ensureUnique<lt>(formattedMessage)) {
      return;
    }
    if constexpr (outPutType == Output::StdConsole) {
      getConsole()->info(formattedMessage);
    } else if constexpr (outPutType == Output::ErrConsole) {
      getErrConsole()->info(formattedMessage);
    }
  }

  /**
   * Formats and prints the message as log-level error
   *
   * @tparam lt pass UNIQUE if this should be printed only once
   * @tparam outPutType pass ERROR if this should be printed on the error console
   * @tparam MSG_t inferred from msg
   * @tparam Args inferred from args
   * @param msg the msg to be formatted and printed
   * @param args the arguments to format the message with
   */
  template <LogType lt = LogType::DEFAULT, Output outPutType = Output::StdConsole, typename MSG_t, typename... Args>
  void error(const MSG_t msg, Args&&... args) {
    const std::string& formattedMessage = fmt::vformat(std::string_view(msg), fmt::make_format_args(args...));
    if (ensureUnique<lt>(formattedMessage)) {
      return;
    }
    if constexpr (outPutType == Output::StdConsole) {
      getConsole()->error(formattedMessage);
    } else if constexpr (outPutType == Output::ErrConsole) {
      getErrConsole()->error(formattedMessage);
    }
  }

  /**
   * Formats and prints the message as log-level debug
   *
   * @tparam lt pass UNIQUE if this should be printed only once
   * @tparam outPutType pass ERROR if this should be printed on the error console
   * @tparam MSG_t inferred from msg
   * @tparam Args inferred from args
   * @param msg the msg to be formatted and printed
   * @param args the arguments to format the message with
   */
  template <LogType lt = LogType::DEFAULT, Output outPutType = Output::StdConsole, typename MSG_t, typename... Args>
  void debug(const MSG_t msg, Args&&... args) {
    const std::string& formattedMessage = fmt::vformat(std::string_view(msg), fmt::make_format_args(args...));
    if (ensureUnique<lt>(formattedMessage)) {
      return;
    }
    if constexpr (outPutType == Output::StdConsole) {
      getConsole()->debug(formattedMessage);
    } else if constexpr (outPutType == Output::ErrConsole) {
      getErrConsole()->debug(formattedMessage);
    }
  }

  /**
   * Formats and prints the message as log-level warn
   *
   * @tparam lt pass UNIQUE if this should be printed only once
   * @tparam outPutType pass ERROR if this should be printed on the error console
   * @tparam MSG_t inferred from msg
   * @tparam Args inferred from args
   * @param msg the msg to be formatted and printed
   * @param args the arguments to format the message with
   */
  template <LogType lt = LogType::DEFAULT, Output outPutType = Output::StdConsole, typename MSG_t, typename... Args>
  void warn(const MSG_t msg, Args&&... args) {
    const std::string& formattedMessage = fmt::vformat(std::string_view(msg), fmt::make_format_args(args...));
    if (ensureUnique<lt>(formattedMessage)) {
      return;
    }
    if constexpr (outPutType == Output::StdConsole) {
      getConsole()->warn(formattedMessage);
    } else if constexpr (outPutType == Output::ErrConsole) {
      getErrConsole()->warn(formattedMessage);
    }
  }

  /**
   * Formats and prints the message as log-level critical
   *
   * @tparam lt pass UNIQUE if this should be printed only once
   * @tparam outPutType pass ERROR if this should be printed on the error console
   * @tparam MSG_t inferred from msg
   * @tparam Args inferred from args
   * @param msg the msg to be formatted and printed
   * @param args the arguments to format the message with
   */
  template <LogType lt = LogType::DEFAULT, Output outPutType = Output::StdConsole, typename MSG_t, typename... Args>
  void critical(const MSG_t msg, Args&&... args) {
    const std::string& formattedMessage = fmt::vformat(std::string_view(msg), fmt::make_format_args(args...));
    if (ensureUnique<lt>(formattedMessage)) {
      return;
    }
    if constexpr (outPutType == Output::StdConsole) {
      getConsole()->critical(formattedMessage);
    } else if constexpr (outPutType == Output::ErrConsole) {
      getErrConsole()->critical(formattedMessage);
    }
  };

  /**
   * Formats and prints the message as log-level trace
   *
   * @tparam lt pass UNIQUE if this should be printed only once
   * @tparam outPutType pass ERROR if this should be printed on the error console
   * @tparam MSG_t inferred from msg
   * @tparam Args inferred from args
   * @param msg the msg to be formatted and printed
   * @param args the arguments to format the message with
   */
  template <LogType lt = LogType::DEFAULT, Output outPutType = Output::StdConsole, typename MSG_t, typename... Args>
  void trace(const MSG_t msg, Args&&... args) {
    const std::string& formattedMessage = fmt::vformat(std::string_view(msg), fmt::make_format_args(args...));
    if (ensureUnique<lt>(formattedMessage)) {
      return;
    }
    if constexpr (outPutType == Output::StdConsole) {
      getConsole()->trace(formattedMessage);
    } else if constexpr (outPutType == Output::ErrConsole) {
      getErrConsole()->trace(formattedMessage);
    }
  }

  /**
   * A shorthand to print a info to the console
   *
   * @tparam MSG_t inferred from msg
   * @tparam Args  inferred from args
   * @param msg the msg to be formatted and printed
   * @param args the arguments to format the message with
   */
  template <typename MSG_t, typename... Args>
  static void logInfo(const MSG_t msg, Args&&... args) {
    metacg::MCGLogger::instance().info<LogType::DEFAULT, Output::StdConsole>(msg, std::forward<Args>(args)...);
  }

  /**
   * A shorthand to print a unique info to the console
   *
   * @tparam MSG_t inferred from msg
   * @tparam Args  inferred from args
   * @param msg the msg to be formatted and printed
   * @param args the arguments to format the message with
   */
  template <typename MSG_t, typename... Args>
  static void logInfoUnique(const MSG_t msg, Args&&... args) {
    metacg::MCGLogger::instance().info<LogType::UNIQUE, Output::StdConsole>(msg, std::forward<Args>(args)...);
  }

  /**
   * A shorthand to print a error to the console
   *
   * @tparam MSG_t inferred from msg
   * @tparam Args  inferred from args
   * @param msg the msg to be formatted and printed
   * @param args the arguments to format the message with
   */
  template <typename MSG_t, typename... Args>
  static void logError(const MSG_t msg, Args&&... args) {
    metacg::MCGLogger::instance().error<LogType::DEFAULT, Output::ErrConsole>(msg, std::forward<Args>(args)...);
  }

  /**
   * A shorthand to print a unique error to the console
   *
   * @tparam MSG_t inferred from msg
   * @tparam Args  inferred from args
   * @param msg the msg to be formatted and printed
   * @param args the arguments to format the message with
   */
  template <typename MSG_t, typename... Args>
  static void logErrorUnique(const MSG_t msg, Args&&... args) {
    metacg::MCGLogger::instance().error<LogType::UNIQUE, Output::ErrConsole>(msg, std::forward<Args>(args)...);
  }

  /**
   * A shorthand to print a unique error to the console
   *
   * @tparam MSG_t inferred from msg
   * @tparam Args  inferred from args
   * @param msg the msg to be formatted and printed
   * @param args the arguments to format the message with
   */
  template <typename MSG_t, typename... Args>
  static void logWarn(const MSG_t msg, Args&&... args) {
    metacg::MCGLogger::instance().warn<LogType::DEFAULT, Output::StdConsole>(msg, std::forward<Args>(args)...);
  }

  /**
   * A shorthand to print a unique error to the console
   *
   * @tparam MSG_t inferred from msg
   * @tparam Args  inferred from args
   * @param msg the msg to be formatted and printed
   * @param args the arguments to format the message with
   */
  template <typename MSG_t, typename... Args>
  static void logWarnUnique(const MSG_t msg, Args&&... args) {
    metacg::MCGLogger::instance().warn<LogType::UNIQUE, Output::StdConsole>(msg, std::forward<Args>(args)...);
  }

  /**
   * Resets the uniqueness-property for all messages.
   * Any message that has been previously logged as unique can now appear again
   *
   * @return the number of messages that now can appear again
   */
  size_t resetUniqueCache(){
    const size_t deletedEntries=alreadyPrintedMessages.size();
    alreadyPrintedMessages.clear();
    return deletedEntries;
  }

 private:
  MCGLogger() : console(spdlog::stdout_color_mt("console")), errconsole(spdlog::stderr_color_mt("errconsole")) {}

  template <LogType lt>
  inline bool ensureUnique(const std::string& formattedMessage) {
    if constexpr (lt == LogType::UNIQUE) {
      const size_t msg_hash = std::hash<std::string>()(formattedMessage);
      // If we found the msg, we just return
      if (alreadyPrintedMessages.find(msg_hash) != alreadyPrintedMessages.end()) {
        return true;
      }
      alreadyPrintedMessages.insert(std::hash<std::string>()(formattedMessage));
    }
    return false;
  }

  std::shared_ptr<spdlog::logger> console;
  std::shared_ptr<spdlog::logger> errconsole;
  std::unordered_set<size_t> alreadyPrintedMessages;
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

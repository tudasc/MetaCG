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
  enum class Output { DEFAULT, ERROR };

  template <Output outPutType=Output::DEFAULT, LogType lt = LogType::DEFAULT, typename MSG_t, typename... Args>
  void info(const MSG_t msg, Args&&... args) {
    const std::string& formattedMessage = fmt::format(msg, args...);
    if (uniqueify<lt>(formattedMessage)){
      return ;
    }
    if constexpr (outPutType==Output::DEFAULT){
      getConsole()->info(formattedMessage);
    }else if constexpr (outPutType==Output::ERROR){
      getErrConsole()->info(formattedMessage);
    }
  };

  template <Output outPutType=Output::DEFAULT, LogType lt = LogType::DEFAULT, typename MSG_t, typename... Args>
  void error(const MSG_t msg, Args&&... args) {
    const std::string& formattedMessage = fmt::format(msg, std::forward<Args>(args)...);
    if (uniqueify<lt>(formattedMessage)){
      return ;
    }
    if constexpr (outPutType==Output::DEFAULT){
      getConsole()->error(formattedMessage);
    }else if constexpr (outPutType==Output::ERROR){
      getErrConsole()->error(formattedMessage);
    }
  };

  template <Output outPutType=Output::DEFAULT, LogType lt = LogType::DEFAULT, typename MSG_t, typename... Args>
  void debug(const MSG_t msg, Args&&... args) {
    const std::string& formattedMessage = fmt::format(msg, std::forward<Args>(args)...);
    if (uniqueify<lt>(formattedMessage)){
      return ;
    }
    if constexpr (outPutType==Output::DEFAULT){
      getConsole()->debug(formattedMessage);
    }else if constexpr (outPutType==Output::ERROR){
      getErrConsole()->debug(formattedMessage);
    }
  };

  template <Output outPutType=Output::DEFAULT, LogType lt = LogType::DEFAULT, typename MSG_t, typename... Args>
  void warn(const MSG_t msg, Args&&... args) {
    const std::string& formattedMessage = fmt::format(msg, std::forward<Args>(args)...);
    if (uniqueify<lt>(formattedMessage)){
      return ;
    }
    if constexpr (outPutType==Output::DEFAULT){
      getConsole()->warn(formattedMessage);
    }else if constexpr (outPutType==Output::ERROR){
      getErrConsole()->warn(formattedMessage);
    }
  };

  template <Output outPutType=Output::DEFAULT, LogType lt = LogType::DEFAULT, typename MSG_t, typename... Args>
  void critical(const MSG_t msg, Args&&... args) {
    const std::string& formattedMessage = fmt::format(msg, std::forward<Args>(args)...);
    if (uniqueify<lt>(formattedMessage)){
      return ;
    }
    if constexpr (outPutType==Output::DEFAULT){
      getConsole()->critical(formattedMessage);
    }else if constexpr (outPutType==Output::ERROR){
      getErrConsole()->critical(formattedMessage);
    }
  };

  template <Output outPutType=Output::DEFAULT, LogType lt = LogType::DEFAULT, typename MSG_t, typename... Args>
  void trace(const MSG_t msg, Args&&... args) {
    const std::string& formattedMessage = fmt::format(msg, std::forward<Args>(args)...);
    if (uniqueify<lt>(formattedMessage)){
      return ;
    }
    if constexpr (outPutType==Output::DEFAULT){
      getConsole()->trace(formattedMessage);
    }else if constexpr (outPutType==Output::ERROR){
      getErrConsole()->trace(formattedMessage);
    }
  };

 private:
  MCGLogger() : console(spdlog::stdout_color_mt("console")), errconsole(spdlog::stderr_color_mt("errconsole")) {}


  template <LogType lt>
  inline bool uniqueify(const std::string& formattedMessage) {
    if constexpr (lt == LogType::UNIQUE) {
      const size_t msg_hash = std::hash<std::string>()(formattedMessage);
      //If we found the msg, we just return
      if (allreadyPrintedMessages.find(msg_hash) != allreadyPrintedMessages.end()) {
        return true;
      }
      allreadyPrintedMessages.insert(std::hash<std::string>()(formattedMessage));
    }
    return false;
  }

  std::shared_ptr<spdlog::logger> console;
  std::shared_ptr<spdlog::logger> errconsole;

  std::unordered_set<size_t> allreadyPrintedMessages;
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
#ifndef PGIS_UNITTEST_LOGGERUTIL_H
#define PGIS_UNITTEST_LOGGERUTIL_H

#include "spdlog/sinks/stdout_color_sinks.h"
#include "spdlog/spdlog.h"

namespace loggerutil {
inline void getLogger() {
  auto c = spdlog::get("console");
  if (!c) {
    c = spdlog::stdout_color_mt("console");
  }
  auto e = spdlog::get("errconsole");
  if (!e) {
    e = spdlog::stderr_color_mt("errconsole");
  }
}
}  // namespace loggerutil

#endif

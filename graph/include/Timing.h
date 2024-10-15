/**
 * File: Timing.h
 * License: Part of the metacg project. Licensed under BSD 3 clause license. See LICENSE.txt file at
 * https://github.com/tudasc/metacg/LICENSE.txt
 */

#ifndef METACG_PGIS_TIMING_H
#define METACG_PGIS_TIMING_H

#include "LoggerUtil.h"
#include <chrono>
#include <utility>

namespace metacg {
class RuntimeTimer {
 public:
  using clock = std::chrono::high_resolution_clock;
  using tp = clock::time_point;
  using duration = clock::duration;

  explicit RuntimeTimer(std::string regionName, bool printOnDestruction = true)
      : region(std::move(regionName)), printOnDestruct(printOnDestruction), start(clock::now()) {}
  ~RuntimeTimer() {
    if (printOnDestruct) {
      stop();
      auto durInSeconds = getTimePassed();
      MCGLogger::instance().getConsole()->info("The region \"{}\" required {} seconds to finish.", region,
                                               durInSeconds.count());
    }
  }

  void stop() { end = clock::now(); }

  std::chrono::seconds getTimePassed() {
    auto dur = end - start;
    auto durInSeconds = std::chrono::duration_cast<std::chrono::seconds>(dur);
    return durInSeconds;
  }

 private:
  const std::string region;
  const bool printOnDestruct;
  const tp start;
  tp end;
};
}  // namespace metacg

#endif

/**
 * File: ticToc.h
 *
 * Author: Andrew Stein
 * Date:   08/31/2018
 *
 * Description: Lightweight scoped tic/toc timing mechanism.
 *              Provides both wall time and cpu time durations, both in milliseconds.
 *
 * Copyright: Anki, Inc. 2018
 **/

#ifndef __Anki_Util_Time_TicToc_H__
#define __Anki_Util_Time_TicToc_H__

#include <chrono>
#include <stdint.h>
#include <time.h>

namespace Anki {
namespace Util {
namespace Time {

using ClockType  = std::chrono::high_resolution_clock;
using TimePoint  = std::pair<ClockType::time_point, timespec>;
using Resolution = std::chrono::milliseconds;
static_assert(ClockType::is_steady, "ClockType should be steady");
  
struct Duration {
  uint64_t wallTime_ms;
  uint64_t cpuTime_ms;

  std::string ToString() const {
    return "WallTime:" + std::to_string(wallTime_ms) + "ms CpuTime:" + std::to_string(cpuTime_ms) + "ms";
  }
};

inline TimePoint Tic()
{
  struct timespec cpu_now;
  clock_gettime(CLOCK_THREAD_CPUTIME_ID, &cpu_now);
  return std::make_pair(ClockType::now(), cpu_now);
}

inline Duration Toc(TimePoint tic)
{
  auto const toc = Tic();
  const auto wallTime_ms = std::chrono::duration_cast<Resolution>(toc.first - tic.first).count();
  const auto cpuTime_ms = (toc.second.tv_nsec - tic.second.tv_nsec) / 1000000; // Convert to milliseconds
  const Duration d{
    .wallTime_ms = static_cast<uint64_t>(wallTime_ms),
    .cpuTime_ms  = static_cast<uint64_t>(cpuTime_ms),
  };
  return d;
}

} // namespace Time
} // namespace Util
} // namespace Anki

#endif /* __Anki_Util_Time_TicToc_H__ */


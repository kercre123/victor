/**
 * File: durationStats.h
 *
 * Author: Andrew Stein
 * Date:   08/31/2018
 *
 * Description:
 *
 * Copyright: Anki, Inc. 2018
 **/

#ifndef __Anki_Util_Time_DurationStats_H__
#define __Anki_Util_Time_DurationStats_H__

#include "util/math/math.h"
#include "util/stats/statsAccumulator.h"
#include "util/time/ticToc.h"

#include <cmath>

namespace Anki {
namespace Util {
namespace Time {
    
class DurationStats
{
public:
  DurationStats() = default;
  
  void AddIfGTZero(const Duration& d)
  {
    _cpuTimeStats.AddIfGTZero(d.cpuTime_ms);
    _wallTimeStats.AddIfGTZero(d.wallTime_ms);
  }
  
  // True if the last value of either CPU or Wall time is > 0
  bool IsLastGTZero() const {
    return (Util::IsFltGTZero(_cpuTimeStats.GetLastVal()) ||
            Util::IsFltGTZero(_wallTimeStats.GetLastVal()));
  }
  
  Duration GetLast() const {
    return Duration{
      .wallTime_ms = (uint64_t)std::round(_wallTimeStats.GetLastVal()),
      .cpuTime_ms  = (uint64_t)std::round(_cpuTimeStats.GetLastVal()),
    };
  }
  
  const Stats::StatsAccumulator& GetCpuStats() const { return _cpuTimeStats; }
  const Stats::StatsAccumulator& GetWallStats() const { return _wallTimeStats; }
  
  void Clear() {
    _cpuTimeStats.Clear();
    _wallTimeStats.Clear();
  }
  
private:
  Stats::StatsAccumulator _cpuTimeStats;
  Stats::StatsAccumulator _wallTimeStats;
};

} // namespace Time
} // namespace Util
} // namespace Anki

#endif /* __Anki_Util_Time_DurationStats_H__ */


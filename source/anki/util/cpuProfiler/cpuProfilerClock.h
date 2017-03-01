/**
 * File: cpuProfilerClock
 *
 * Author: Mark Wesley
 * Created: 07/30/16
 *
 * Description: Wrapper and helpers for clock used by CpuProfiler
 *
 * Copyright: Anki, Inc. 2016
 *
 **/


#ifndef __Util_CpuProfiler_CpuProfilerClock_H__
#define __Util_CpuProfiler_CpuProfilerClock_H__


#include "util/cpuProfiler/cpuProfilerSettings.h"
#include <chrono>


#if ANKI_CPU_PROFILER_ENABLED


namespace Anki {
namespace Util {


using CpuProfileClock = std::chrono::high_resolution_clock;


inline double CalcDuration_ms(const CpuProfileClock::time_point& t1, const CpuProfileClock::time_point& t2)
{
  std::chrono::duration<double, std::milli> duration_ms = t2 - t1;
  return duration_ms.count();
}

    
} // end namespace Util
} // end namespace Anki


#endif // ANKI_CPU_PROFILER_ENABLED


#endif // __Util_CpuProfiler_CpuProfilerClock_H__

/**
 * File: cpuProfilerSettings
 *
 * Author: Mark Wesley
 * Created: 07/30/16
 *
 * Description: Settings for CpuProfiler (e.g. if it should be enabled or not)
 *
 * Copyright: Anki, Inc. 2016
 *
 **/


#ifndef __Util_CpuProfiler_CpuProfilerSettings_H__
#define __Util_CpuProfiler_CpuProfilerSettings_H__


#include "util/global/globalDefinitions.h"
#include <stdint.h>


#if ANKI_PROFILING_ENABLED
  #define ANKI_CPU_PROFILER_ENABLED 1
#else
  #define ANKI_CPU_PROFILER_ENABLED 0
#endif


#if ANKI_CPU_PROFILER_ENABLED
  #define ANKI_CPU_PROFILER_ENABLED_ONLY(expr)  expr
#else
  #define ANKI_CPU_PROFILER_ENABLED_ONLY(expr)
#endif


// Max number of threads that can be simultaneously profiled by CpuProfiler
// Beware raising this too high - we linear search for threadId (so OK for N <= ~16)
const uint32_t kCpuProfilerMaxThreads = 4;


#endif // __Util_CpuProfiler_CpuProfilerSettings_H__

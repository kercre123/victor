/**
 * File: cpuProfileSample
 *
 * Author: Mark Wesley
 * Created: 07/30/16
 *
 * Description: Measures a single profile timing (e.g. 1 call of 1 function)
 *
 * Copyright: Anki, Inc. 2016
 *
 **/


#ifndef __Util_CpuProfiler_CpuProfileSample_H__
#define __Util_CpuProfiler_CpuProfileSample_H__


#include "util/cpuProfiler/cpuProfilerClock.h"
#include "util/cpuProfiler/cpuProfilerSettings.h"
#include "util/cpuProfiler/cpuProfileSampleShared.h"


#if ANKI_CPU_PROFILER_ENABLED


namespace Anki {
namespace Util {
  
  
class CpuProfileSample
{
public:
  
  CpuProfileSample()
    : _duration_ms(0.0)
    , _sharedData(nullptr)
    , _startTimePoint()
    , _endTimePoint()
  {
  }
  
  CpuProfileSample(CpuProfileSampleShared* sharedData,
                   const CpuProfileClock::time_point& startTimePoint,
                   const CpuProfileClock::time_point& endTimePoint,
                   double duration_ms)
    : _duration_ms(duration_ms)
    , _sharedData(sharedData)
    , _startTimePoint(startTimePoint)
    , _endTimePoint(endTimePoint)
  {
  }
  
  double GetDuration_ms() const { return _duration_ms; }
  
  const CpuProfileSampleShared& GetSharedData()          const { return *_sharedData; }
  const CpuProfileClock::time_point& GetStartTimePoint() const { return _startTimePoint; }
  const CpuProfileClock::time_point& GetEndTimePoint()   const { return _endTimePoint; }
  
  const char* GetName() const { return GetSharedData().GetName(); }
  
private:
  
  double                      _duration_ms;
  CpuProfileSampleShared*     _sharedData;
  CpuProfileClock::time_point _startTimePoint;
  CpuProfileClock::time_point _endTimePoint;
};
  
  
} // end namespace Util
} // end namespace Anki

  
#endif // ANKI_CPU_PROFILER_ENABLED


#endif // __Util_CpuProfiler_CpuProfileSample_H__


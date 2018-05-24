/**
 * File: cpuThreadProfile
 *
 * Author: Mark Wesley
 * Created: 07/30/16
 *
 * Description: Profile for 1 tick on 1 thread
 *
 * Copyright: Anki, Inc. 2016
 *
 **/


#ifndef __Util_CpuProfiler_CpuThreadProfile_H__
#define __Util_CpuProfiler_CpuThreadProfile_H__


#include "util/cpuProfiler/cpuProfilerSettings.h"
#include "util/cpuProfiler/cpuProfileSample.h"
#include "util/logging/logging.h"
#include <assert.h>
#include <vector>
#include <functional>

#if ANKI_CPU_PROFILER_ENABLED

namespace Json {
class Value;
}

namespace Anki {

namespace WebService {
  class WebService;
}

namespace Util {


class CpuThreadProfile
{
public:
  
  explicit CpuThreadProfile(size_t numSamples = 0)
    : _samples(numSamples)
    , _startTimePoint()
    , _endTimePoint()
    , _tickDuration(0.0)
    , _tickNum(0)
  {
    _recentTickCount[0] = 0;
    _recentTickCount[1] = 0;
  }
  
  ~CpuThreadProfile() {}
  
  void Init(uint32_t numSamples)
  {
    _samples.clear();
    _samples.reserve(numSamples);
    _tickDuration = 0.0;
    _tickNum = 0;
    _recentTickCount[0] = 0;
    _recentTickCount[1] = 0;
  }
  
  bool AddSample(const CpuProfileClock::time_point& start,
                 const CpuProfileClock::time_point& end,
                 const double duration_ms,
                 CpuProfileSampleShared& profileStats)
  {
    if (_started && (_samples.size() < _samples.capacity()))
    {
      _samples.emplace_back(&profileStats, start, end, duration_ms);
      return true;
    }
    else
    {
      return false;
    }
  }
  
  void StartTick()
  {
    assert(!_started);
    _samples.clear();
    _started = true;
    ++_tickNum;
    ++_recentTickCount[0];
    ++_recentTickCount[1];
    _startTimePoint = CpuProfileClock::now();
  }
  
  void EndTick();
  
  void SortSamples();       // optional, needed for LogProfile() to work correctly
  void LogProfile(uint32_t threadIndex) const;
  
  void LogAllCalledSamples(uint32_t threadIndex, const std::vector<CpuProfileSampleShared*>& samplesCalledFromThread) const;
  void SaveChromeTracingProfile(FILE* fp, uint32_t threadIndex) const;
  void PublishToWebService(const std::function<void(const Json::Value&)>& callback, const char* threadName, uint32_t threadIndex, const std::vector<CpuProfileSampleShared*>& samplesCalledFromThread) const;

  uint32_t GetTickNum() const { return _tickNum; }
  
  const CpuProfileClock::time_point& GetStartTimePoint() const { return _startTimePoint; }
  const CpuProfileClock::time_point& GetEndTimePoint()   const { return _endTimePoint; }
  
  size_t GetSampleCount() const { return _samples.size(); }
  size_t GetSampleCapacity() const { return _samples.capacity(); }
  const CpuProfileSample& GetSample(size_t index) const { return _samples[index]; }
  
  double GetTickDuration() const { return _tickDuration; }
  
  bool HasStarted() const { return _started; }
  
  uint32_t GetRecentTickCount(uint32_t index) const
  {
    assert(index < 2);
    return (index < 2) ? _recentTickCount[index] : 0;
  }
  
  uint32_t GetLargestRecentTickCount() const { return _recentTickCount[GetIndexForLargestRecentTickCount()]; }
  
  void ResetRecentTickCount(uint32_t index)
  {
    assert(index < 2);
    if (index < 2)
    {
      _recentTickCount[index] = 0;
    }
  }
  
  uint32_t GetIndexForSmallestRecentTickCount() const { return _recentTickCount[0] < _recentTickCount[1] ? 0 : 1; }
  uint32_t GetIndexForLargestRecentTickCount()  const { return _recentTickCount[0] < _recentTickCount[1] ? 1 : 0; }
  
private:
  
  std::vector<CpuProfileSample> _samples;
  CpuProfileClock::time_point   _startTimePoint;
  CpuProfileClock::time_point   _endTimePoint;
  double                        _tickDuration;
  uint32_t                      _tickNum;
  uint32_t                      _recentTickCount[2];
  bool                          _started;
};


} // end namespace Util
} // end namespace Anki


#endif // ANKI_CPU_PROFILER_ENABLED


#endif // __Util_CpuProfiler_CpuThreadProfiler_H__

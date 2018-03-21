/**
 * File: cpuThreadProfiler
 *
 * Author: Mark Wesley
 * Created: 07/30/16
 *
 * Description: Profiles 1 thread (so safely only accessed by that 1 thread)
 *
 * Copyright: Anki, Inc. 2016
 *
 **/


#ifndef __Util_CpuProfiler_CpuThreadProfiler_H__
#define __Util_CpuProfiler_CpuThreadProfiler_H__


#include "util/cpuProfiler/cpuProfilerSettings.h"
#include "util/cpuProfiler/cpuThreadId.h"
#include "util/cpuProfiler/cpuThreadProfile.h"
#include <cstdio>
#include <vector>


#if ANKI_CPU_PROFILER_ENABLED


#define ANKI_CPU_PROFILER_WARN_ON_TINY_SAMPLES     0   // Enable to track down functions that are too quick to be worth profiling
#define ANKI_CPU_PROFILER_WARN_ON_SKIPPED_SAMPLES  0   // Enable to track down functions that aren't being profiled


namespace Anki {
namespace Util {


class CpuThreadProfiler
{
public:
  
  static constexpr const uint32_t kThreadIndexInvalid = 0xffffffff;
  static constexpr const char*    kThreadNameInvalid = "";
  static constexpr const uint32_t kLogFrequencyNever = 0xffffffff;
  
  CpuThreadProfiler();
  
  ~CpuThreadProfiler()
  {
  }
  
  void Reset();
  
  void Init(CpuThreadId threadId, uint32_t threadIndex, const char* threadName, double maxTickTime_ms,
            uint32_t logFrequency, const CpuProfileClock::time_point& baseTimePoint, uint32_t sampleCount = 512);
  
  static void SetChromeTracingFile(const char* tracingFile);
  void ReuseThread(CpuThreadId threadId);
  
  void SortProfile() { _currentProfile.SortSamples(); } // optional, needed for LogProfile() to work correctly
  void LogProfile() const;
  void SortAndLogProfile();
  
  void LogAllCalledSamples() const;

  void AddSample(const CpuProfileClock::time_point& start,
                 const CpuProfileClock::time_point& end,
                 CpuProfileSampleShared& sharedData)
  {
    const double duration_ms = CalcDuration_ms(start, end);
    
    if (duration_ms > sMinSampleDuration_ms)
    {
      if (!_currentProfile.AddSample(start, end, duration_ms, sharedData))
      {
      #if ANKI_CPU_PROFILER_WARN_ON_SKIPPED_SAMPLES
        WarnOfSkippedProfileSample(sharedData, duration_ms);
      #endif // ANKI_CPU_PROFILER_WARN_ON_SKIPPED_SAMPLES
      }
    }
    else
    {
    #if ANKI_CPU_PROFILER_WARN_ON_TINY_SAMPLES
      WarnOfTinyProfileSample(sharedData, duration_ms);
    #endif // ANKI_CPU_PROFILER_WARN_ON_TINY_SAMPLES
    }
    
    sharedData.AddSample(_threadIndex, duration_ms);
    
    if (!sharedData.IsCalledFromThread(_threadIndex))
    {
      sharedData.SetIsCalledFromThread(_threadIndex, true);
      _samplesCalledFromThread.push_back(&sharedData);
    }
  }
  
  void StartTick();
  void EndTick();
  
  const CpuThreadProfile& GetCurrentProfile() const { return _currentProfile; } // NOTE: Unsafe to call from another thread whilst this is being updated
  
  bool IsSameThread(CpuThreadId threadId) const { return AreCpuThreadIdsEqual(_threadId, threadId); }
  
  const char* GetThreadName() const { return _threadName; }
  uint32_t    GetThreadIndex() const { return _threadIndex; }
    
  double    GetMaxTickTime_ms() const { return _maxTickTime_ms; };
  void      SetMaxTickTime_ms(double newVal) { _maxTickTime_ms = newVal; };
  
  uint32_t  GetLogFrequency() const { return _logFrequency; };
  void      SetLogFrequency(uint32_t newVal) { _logFrequency = newVal; };
  
  double      GetTimeSinceBase_ms(const CpuProfileClock::time_point& timePoint) const
  {
    return CalcDuration_ms(_baseTimePoint, timePoint);
  }
  
  bool HasBeenDeleted() const { return _hasBeenDeleted; }
  void SetHasBeenDeleted(bool newVal=true) { _hasBeenDeleted = newVal; }
  
  void RequestProfile(bool newVal = true) { _profileRequested = newVal; }
  
  static void SetMinSampleDuration_ms(double newVal) { sMinSampleDuration_ms = newVal; }
  
  bool HasStaleSettings() const { return _hasStaleSettings; }
  void SetHasStaleSettings(bool newVal = true) { _hasStaleSettings = newVal; }
  
private:
  
  void WarnOfSkippedProfileSample(const CpuProfileSampleShared& sharedData, double duration_ms);
  void WarnOfTinyProfileSample(const CpuProfileSampleShared& sharedData, double duration_ms);
  
  static double sMinSampleDuration_ms; // ignore trivial samples below this
  static FILE* _chromeTracingFile;

  std::vector<CpuProfileSampleShared*>  _samplesCalledFromThread;
  const char*                 _threadName;
  CpuThreadProfile            _currentProfile;
  CpuThreadId                 _threadId;
  CpuProfileClock::time_point _baseTimePoint;  // a common time point shared by all threads, before any thread started
  double                      _maxTickTime_ms; // Max tick time before forcing tick to be logged
  uint32_t                    _threadIndex;    // index of this thread in the profiler (for simple TLS)
  uint32_t                    _logFrequency;   // log profile every _logFrequency ticks
  bool                        _hasBeenDeleted;
  bool                        _profileRequested;
  bool                        _hasStaleSettings;
};

  
  
} // end namespace Util
} // end namespace Anki


#endif // ANKI_CPU_PROFILER_ENABLED


#endif // __Util_CpuProfiler_CpuThreadProfiler_H__

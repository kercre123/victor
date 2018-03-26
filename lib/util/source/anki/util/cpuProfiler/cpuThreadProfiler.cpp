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


#include "util/cpuProfiler/cpuThreadProfiler.h"
#include "util/console/consoleInterface.h"
#include "util/logging/logging.h"


#if ANKI_CPU_PROFILER_ENABLED


namespace Anki {
namespace Util {

  
CONSOLE_VAR(bool, kProfilerLogSlowTicks, "CpuProfiler", false);
  
  
double CpuThreadProfiler::sMinSampleDuration_ms = 0.01; // ignore trivial samples below this

FILE* CpuThreadProfiler::_chromeTracingFile = nullptr;

CpuThreadProfiler::CpuThreadProfiler()
  : _threadName(kThreadNameInvalid)
  , _currentProfile()
  , _threadId(kCpuThreadIdInvalid)
  , _baseTimePoint()
  , _maxTickTime_ms(0.0)
  , _threadIndex(kThreadIndexInvalid)
  , _logFrequency(kLogFrequencyNever)
  , _hasBeenDeleted(false)
  , _profileRequested(false)
  , _hasStaleSettings(false)
{
}


void CpuThreadProfiler::Reset()
{
  for (CpuProfileSampleShared* sample : _samplesCalledFromThread)
  {
    sample->ResetForThread(_threadIndex);
  }
  _samplesCalledFromThread.clear();
  
  Init(kCpuThreadIdInvalid, kThreadIndexInvalid, kThreadNameInvalid, 0.0,
       kLogFrequencyNever, CpuProfileClock::time_point());
}


void CpuThreadProfiler::Init(CpuThreadId threadId, uint32_t threadIndex, const char* threadName, double maxTickTime_ms,
          uint32_t logFrequency, const CpuProfileClock::time_point& baseTimePoint, uint32_t sampleCount)
{
  _threadName = threadName;
  _currentProfile.Init(sampleCount);
  _threadId = threadId;
  _baseTimePoint = baseTimePoint;
  _maxTickTime_ms = maxTickTime_ms;
  _threadIndex = threadIndex;
  _logFrequency = logFrequency;
  _hasBeenDeleted = false;
  _profileRequested = false;
  _hasStaleSettings = false;
}
  
  
void CpuThreadProfiler::SetChromeTracingFile(const char* tracingFile) {
  if (_chromeTracingFile) {
    fclose(_chromeTracingFile);
    _chromeTracingFile = nullptr;
  }

  _chromeTracingFile = fopen(tracingFile, "w");
  if (_chromeTracingFile) {
    fprintf(_chromeTracingFile, "{\"displayTimeUnit\": \"ms\",");
    fprintf(_chromeTracingFile, "\"ts_epoch\": %lld,", std::chrono::duration_cast<std::chrono::milliseconds>(CpuProfileClock::now().time_since_epoch()).count());
    fprintf(_chromeTracingFile, "\"app_epoch\": %lld,\n", std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count());
    fprintf(_chromeTracingFile, "\"traceEvents\": [");
  }
}


void CpuThreadProfiler::ReuseThread(CpuThreadId threadId)
{
  assert(_hasBeenDeleted);
  _threadId = threadId;
  _hasBeenDeleted = false;
}


void CpuThreadProfiler::StartTick()
{
  _currentProfile.StartTick();
}


void CpuThreadProfiler::EndTick()
{
  _currentProfile.EndTick();
  
  const double tickDuration = _currentProfile.GetTickDuration();
  const bool isSlowTick = (tickDuration > _maxTickTime_ms);
  const bool shouldLogTickFreq = (_logFrequency != kLogFrequencyNever) &&
                                 ((_logFrequency == 0) || ((_currentProfile.GetTickNum() % _logFrequency) == 0));
  
  if (_profileRequested || shouldLogTickFreq || (isSlowTick && kProfilerLogSlowTicks))
  {
    if (isSlowTick)
    {
      PRINT_CH_INFO("CpuProfiler", "SlowTick", "Thread %u '%s' Took %.3f ms (> %.3f ms)",
                    GetThreadIndex(), GetThreadName(), tickDuration, _maxTickTime_ms);
    }
    SortAndLogProfile();
    LogAllCalledSamples();
    
    _profileRequested = false;
  }
  else
  {
    //PRINT_CH_INFO("CpuProfiler", "NotLogging", "Thread %u '%s' tick %u Took %.3f ms",
    //              GetThreadIndex(), GetThreadName(), _currentProfile.GetTickNum(), _currentProfile.GetTickDuration() );
  }
  
  #if ANKI_CPU_PROFILE_TRACK_SAMPLE_STATS
  {
    // Every N ticks we reset the largest of the dual accumulators
    // This allows us to only track the most recent N..2N values without having to maintain a rolling buffer
    
    const uint32_t kMaxTicksForRecentStats = 100;
    const uint32_t smallestTickCountIndex = _currentProfile.GetIndexForSmallestRecentTickCount();
    const uint32_t smallestTickCount = _currentProfile.GetRecentTickCount(smallestTickCountIndex);
    
    if (smallestTickCount > kMaxTicksForRecentStats)
    {
      const uint32_t largestTickCountIndex = (smallestTickCountIndex == 0) ? 1 : 0;
      
      // Reset the largest set
      // (the smallest set will now be 0, and it will take kMaxTicksForRecentStats until we reset again)
      
      for (CpuProfileSampleShared* sample : _samplesCalledFromThread)
      {
        CpuSampleStats& sampleStats = sample->GetSampleStats(_threadIndex);
        sampleStats.ResetRecent(largestTickCountIndex);
      }
      
      _currentProfile.ResetRecentTickCount(largestTickCountIndex);
    }
  }
  #endif // ANKI_CPU_PROFILE_TRACK_SAMPLE_STATS
}


void CpuThreadProfiler::LogProfile() const
{
  if (_chromeTracingFile) {
      _currentProfile.SaveChromeTracingProfile(_chromeTracingFile, _threadIndex);
  } else {
    PRINT_CH_INFO("CpuProfiler", "LogProfile", "Thread %u '%s' tick %u [%.3f ms from start %.3f, end %.3f]",
                  GetThreadIndex(), GetThreadName(), _currentProfile.GetTickNum(),
                  _currentProfile.GetTickDuration(),
                  GetTimeSinceBase_ms(_currentProfile.GetStartTimePoint()),
                  GetTimeSinceBase_ms(_currentProfile.GetEndTimePoint()));
    _currentProfile.LogProfile(_threadIndex);
  }
}
  
  
void CpuThreadProfiler::SortAndLogProfile()
{
  SortProfile();
  LogProfile();
}
  
  
void CpuThreadProfiler::LogAllCalledSamples() const
{
  if (!_chromeTracingFile) {
    PRINT_CH_INFO("CpuProfiler", "LogAllCalledSamples", "Thread %u '%s' tick %u",
                  GetThreadIndex(), GetThreadName(), _currentProfile.GetTickNum());

    _currentProfile.LogAllCalledSamples(_threadIndex, _samplesCalledFromThread);
  }
}

  
void CpuThreadProfiler::WarnOfTinyProfileSample(const CpuProfileSampleShared& sharedData, double duration_ms)
{
  PRINT_NAMED_WARNING("CpuThreadProfiler.IgnoreTinySample", "Ignoring Tiny Sample '%s' took %f ms (<= %f)",
                      sharedData.GetName(), duration_ms, sMinSampleDuration_ms);
}

  
void CpuThreadProfiler::WarnOfSkippedProfileSample(const CpuProfileSampleShared& sharedData, double duration_ms)
{
  PRINT_NAMED_WARNING("CpuThreadProfiler.SkippedSample", "Skipped Sample '%s' took %f ms%s (%zu/%zu samples)",
                      sharedData.GetName(), duration_ms,
                      _currentProfile.HasStarted() ? "" : " TICK NOT STARTED!",
                      _currentProfile.GetSampleCount(), _currentProfile.GetSampleCapacity());
}


} // end namespace Util
} // end namespace Anki



#endif // ANKI_CPU_PROFILER_ENABLED

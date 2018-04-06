/**
 * File: cpuProfiler
 *
 * Author: Mark Wesley
 * Created: 06/10/16
 *
 * Description: A lightweight (minimal skewing) CPU profiler
 *
 * Copyright: Anki, Inc. 2016
 *
 **/


#ifndef __Util_CpuProfiler_CpuProfiler_H__
#define __Util_CpuProfiler_CpuProfiler_H__


#include "util/cpuProfiler/cpuProfilerClock.h"
#include "util/cpuProfiler/cpuProfilerSettings.h"
#include "util/cpuProfiler/cpuProfileSampleShared.h"
#include "util/cpuProfiler/cpuThreadId.h"
#include "util/cpuProfiler/cpuThreadProfiler.h"
#include "util/logging/logging.h"
#include <assert.h>
#include <vector>

#if ANKI_CPU_PROFILER_ENABLED

  #define ANKI_CPU_CONSOLEVARGROUP "CpuProfiler"

  #define ANKI_CPU_PROFILER_WARN_ON_NO_PROFILER     0   // Enable to track down calls on untracked threads

  namespace Anki {
  namespace Util {


  // ================================================================================
  // CpuProfiler
  
  class CpuProfiler
  {
  public:
    static const char* CpuProfilerLogging() {
      return "Never,1ms,2ms,4ms,8ms,16ms,32ms";
    }
    
    static uint32_t CpuProfilerLoggingTime(int option) {
      static uint32_t times[] = {Anki::Util::CpuThreadProfiler::kLogFrequencyNever, 1, 2, 4, 8, 16, 32};
      return times[option];
    }

    CpuProfiler()
      : _threadProfilerCount(0)
      , _creationTimePoint(CpuProfileClock::now())
    {
    }
    
    ~CpuProfiler()
    {
    }
    
    void Reset(); // Unsafe to call when any profiled threads are ticking
    
    static CpuProfiler& GetInstance();
    
    CpuThreadProfiler*  GetThreadProfiler(CpuThreadId threadId); // returns nullptr if not already created
    CpuThreadProfiler*  GetThreadProfilerByName(const char* threadName); // This is the threadName from ANKI_CPU_TICK() not the OS thread name
    
    // GetOrAddThreadProfiler: return same as GetThreadProfiler() if already exists, otherwise attempts to add and
    // return that (returns nullptr on failure to add)
    CpuThreadProfiler*  GetOrAddThreadProfiler(CpuThreadId threadId, const char* threadName, double maxTickTime_ms, uint32_t logFreq);
    
    void RemoveThreadProfiler(CpuThreadId threadId);
    
    // Helper static method
    static CpuThreadProfiler* GetOrAddCurrentThreadProfiler(const char* threadName, double maxTickTime_ms, uint32_t logFreq)
    {
      return GetInstance().GetOrAddThreadProfiler( GetCurrentThreadId(), threadName, maxTickTime_ms, logFreq );
    }
    
    static CpuThreadProfiler* GetCurrentThreadProfiler()
    {
      return GetInstance().GetThreadProfiler( GetCurrentThreadId() );
    }
    
    static void RemoveCurrentThreadProfiler()
    {
      return GetInstance().RemoveThreadProfiler( GetCurrentThreadId() );
    }
    
    uint32_t GetThreadProfilerCount() const { return _threadProfilerCount; }
    const CpuThreadProfiler* GetThreadProfilerByIndex(uint32_t index) const
    {
      return (index < _threadProfilerCount) ? &_threadProfilers[index] : nullptr;
    }
    CpuThreadProfiler* GetThreadProfilerByIndex(uint32_t index)
    {
      return (index < _threadProfilerCount) ? &_threadProfilers[index] : nullptr;
    }
    
  private:
    
    CpuThreadProfiler           _threadProfilers[kCpuProfilerMaxThreads];
    std::atomic<uint32_t>       _threadProfilerCount;
    CpuProfileClock::time_point _creationTimePoint;
    
    void CheckAndUpdateProfiler(CpuThreadProfiler& profiler, double maxTickTime_ms, uint32_t logFreq) const;
  };
  
    
  // ================================================================================
  // Helper Classes and Macros:
  
  
  // ================================================================================
  // ScopedCpuTick
    
  class ScopedCpuTick
  {
  public:
    
    explicit ScopedCpuTick(const char* tickName, float maxTickTime_ms, uint32_t logFreq, bool oneTimeUse = false)
      : _tickName(tickName)
      , _started(false)
      , _isOneTimeUse(oneTimeUse)
    {
      CpuThreadProfiler* profiler = CpuProfiler::GetOrAddCurrentThreadProfiler(tickName, maxTickTime_ms, logFreq);
      if (profiler)
      {
        profiler->StartTick();
        _started = true;
      }
      else
      {
        PRINT_NAMED_WARNING("ScopedCpuTick.FailedToAddProfiler", "Unable To Profile Thread '%s'", tickName);
      }
    }
    
    ~ScopedCpuTick()
    {
      if (_started)
      {
        CpuThreadProfiler* profiler = CpuProfiler::GetCurrentThreadProfiler();
        if (profiler)
        {
          profiler->EndTick();
          if (_isOneTimeUse)
          {
            profiler->SetHasStaleSettings();
          }
        }
        else
        {
          PRINT_NAMED_ERROR("ScopedCpuTick.LostProfiler", "Profiler for '%s' Removed During Tick?", _tickName);
        }
        _started = false;
      }
    }
    
    const char* GetName() const { return _tickName; }

  private:
    
    const char* _tickName;
    bool        _started;
    bool        _isOneTimeUse;
  };

  
  // ================================================================================
  // ScopedCpuProfile
  
  class ScopedCpuProfile
  {
  public:
    
    explicit ScopedCpuProfile(CpuProfileSampleShared& sharedData, bool isActive)
      : _sharedData(sharedData)
      , _active(isActive)
    {
      if (_active)
      {
        _startTime = CpuProfileClock::now();
      }
    }
    
    ~ScopedCpuProfile()
    {
      Stop();
    }
    
    void Stop()
    {
      if (_active)
      {
        CpuThreadProfiler* profiler = CpuProfiler::GetCurrentThreadProfiler();
        if (profiler)
        {
          CpuProfileClock::time_point endTime = CpuProfileClock::now();
          profiler->AddSample(_startTime, endTime, _sharedData);
        }
        else
        {
          // Don't warn before any ticks are running (e.g. on loading)
          if (CpuProfiler::GetInstance().GetThreadProfilerCount() > 0)
          {
          #if ANKI_CPU_PROFILER_WARN_ON_NO_PROFILER
            PRINT_NAMED_WARNING("ScopedCpuTick.NoThreadProfiler",
                                "No Thread Profiler for Thread '%s' sample '%s' - needs an ANKI_CPU_TICK",
                                GetCurrentThreadName().c_str(), _sharedData.GetName());
          #endif // ANKI_CPU_PROFILER_WARN_ON_NO_PROFILER
          }
        }
        _active = false;
      }
    }
    
  private:
    
    CpuProfileSampleShared&     _sharedData;
    CpuProfileClock::time_point _startTime;
    bool                        _active; // allows Stop() to be called manually, and conditional Scoped profilers to be used
  };
  
    
  } // end namespace Util
  } // end namespace Anki


  // Macros for creating unique variable names using __LINE__ macro
  // Needs the multiple levels of indirection so that __LINE__ is expanded to the line number
  #define ANKI_CPU_UNIQUE_VAR_NAME3(varName, lineNum)   varName ## lineNum
  #define ANKI_CPU_UNIQUE_VAR_NAME2(varName, lineNum)   ANKI_CPU_UNIQUE_VAR_NAME3(varName, lineNum)
  #define ANKI_CPU_UNIQUE_VAR_NAME(varName)             ANKI_CPU_UNIQUE_VAR_NAME2(varName, __LINE__)


  // NOTE: Name must be static, we don't copy them!
  //       If we need dynamic strings we can add a ANKI_CPU_MAKE_DYNAMIC_STRING() function to allow a (semi?)permanent
  //       pool of names to be managed and later psudo-"garbage-collected" (cycle the list so the name lasts for e.g. 2 frames)

  #define ANKI_CPU_PROFILE(name)                \
    static Anki::Util::CpuProfileSampleShared   ANKI_CPU_UNIQUE_VAR_NAME(sSharedData)(name); \
    Anki::Util::ScopedCpuProfile                ANKI_CPU_UNIQUE_VAR_NAME(scopedCpuProfile)(ANKI_CPU_UNIQUE_VAR_NAME(sSharedData), true)

  // NOTE: ANKI_CPU_PROFILE_START + ANKI_CPU_PROFILE_STOP still create a scoped timer that will stop when it goes out
  //       of scope (same as ANKI_CPU_PROFILE), it just gives you a handle so you can also stop earlier if desired
  #define ANKI_CPU_PROFILE_START(varName, name) \
    static Anki::Util::CpuProfileSampleShared   ANKI_CPU_UNIQUE_VAR_NAME(sSharedData)(name); \
    Anki::Util::ScopedCpuProfile                varName(ANKI_CPU_UNIQUE_VAR_NAME(sSharedData), true)

  #define ANKI_CPU_PROFILE_STOP(varName)        \
    varName.Stop()

  #define ANKI_CPU_TICK(tickName, maxTickTime_ms, logFreq) \
    Anki::Util::ScopedCpuTick                     ANKI_CPU_UNIQUE_VAR_NAME(scopedCpuTick)(tickName, maxTickTime_ms, logFreq)

  #define ANKI_CPU_TICK_ONE_TIME(tickName) \
    Anki::Util::ScopedCpuTick                     ANKI_CPU_UNIQUE_VAR_NAME(scopedCpuTick)(tickName, 0, 0, true)

  #define ANKI_CPU_REMOVE_THIS_THREAD()         Anki::Util::CpuProfiler::RemoveCurrentThreadProfiler()

#else  // ANKI_CPU_PROFILER_ENABLED


  #define ANKI_CPU_PROFILE(name)
  #define ANKI_CPU_PROFILE_START(varName, name)
  #define ANKI_CPU_PROFILE_STOP(varName)
  #define ANKI_CPU_TICK(tickName, maxTickTime_ms, logFreq)
  #define ANKI_CPU_TICK_ONE_TIME(tickName)
  #define ANKI_CPU_REMOVE_THIS_THREAD()


#endif // ANKI_CPU_PROFILER_ENABLED


#endif // __Util_CpuProfiler_CpuProfiler_H__

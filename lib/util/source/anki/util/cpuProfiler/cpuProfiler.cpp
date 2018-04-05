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


#include "util/cpuProfiler/cpuProfiler.h"
#include "util/console/consoleInterface.h"
#include "util/string/stringHelpers.h"


#if ANKI_CPU_PROFILER_ENABLED


namespace Anki {
namespace Util {


namespace
{
  // Use an anymous-namespaced global rather than a function static because:
  // a) We assume that no calls to profile happen before static initialization, so the global will be constructed in time
  // b) We avoid any race condition from multiple threads calling GetInstance() before the instance if finished construction
  CpuProfiler gProfilerInstance;
} // end anonymous namespace
  
CpuProfiler& CpuProfiler::GetInstance()
{
  return gProfilerInstance;
}
  
  
void CpuProfiler::Reset()
{
  // Primarily meant for resetting state between unit tests
  // Not safe for generic (unsafe whenever any thread is being profiled)
  
  uint32_t profilerCount = _threadProfilerCount;
  for (uint32_t i = 0; i < profilerCount; ++i)
  {
    CpuThreadProfiler& threadProfiler = _threadProfilers[i];
    threadProfiler.Reset();
  }
  
  _threadProfilerCount = 0;
}
  

CpuThreadProfiler* CpuProfiler::GetThreadProfiler(CpuThreadId threadId)
{
  uint32_t profilerCount = _threadProfilerCount;
  for (uint32_t i = 0; i < profilerCount; ++i)
  {
    CpuThreadProfiler& threadProfiler = _threadProfilers[i];
    if (threadProfiler.IsSameThread(threadId))
    {
      return &threadProfiler;
    }
  }
  
  // No profiler for this thread
  return nullptr;
}
  

  
CpuThreadProfiler* CpuProfiler::GetThreadProfilerByName(const char* threadName)
{
  uint32_t profilerCount = _threadProfilerCount;
  for (uint32_t i = 0; i < profilerCount; ++i)
  {
    CpuThreadProfiler& threadProfiler = _threadProfilers[i];
    if (Util::stricmp(threadProfiler.GetThreadName(), threadName) == 0)
    {
      return &threadProfiler;
    }
  }
  
  // No profiler with this threadname
  return nullptr;
}

void CpuProfiler::CheckAndUpdateProfiler(CpuThreadProfiler& profiler, double maxTickTime_ms, uint32_t logFreq) const
{
  // Skip checking for stale settings as these can be changed via console variables
  profiler.SetMaxTickTime_ms(maxTickTime_ms);
  profiler.SetLogFrequency(logFreq);
}

CpuThreadProfiler* CpuProfiler::GetOrAddThreadProfiler(CpuThreadId threadId, const char* threadName,
                                                       double maxTickTime_ms, uint32_t logFreq)
{
  CpuThreadProfiler* existingProfiler = GetThreadProfiler(threadId);
  if (existingProfiler)
  {
    CheckAndUpdateProfiler(*existingProfiler, maxTickTime_ms, logFreq);
    return existingProfiler;
  }
  
  // Not found - try reusing any dead threads with the exact same name
  
  existingProfiler = GetThreadProfilerByName(threadName);
  if (existingProfiler)
  {
    if (existingProfiler->HasBeenDeleted())
    {
      // Reuse this thread
      existingProfiler->ReuseThread(threadId);
      
      CheckAndUpdateProfiler(*existingProfiler, maxTickTime_ms, logFreq);
      
      PRINT_CH_INFO("CpuProfiler", "CpuProfiler.ThreadReused", "Thread %u: '%s' being reused", existingProfiler->GetThreadIndex(), threadName);
      return existingProfiler;
    }
    else
    {
      PRINT_NAMED_WARNING("CpuProfiler.ThreadNameClash", "Multiple threads named '%s' - will not profile duplicate threads!", threadName);
      return nullptr;
    }
  }
  
  // No reusable entries try adding this threadId
  
  const uint32_t newIndex = _threadProfilerCount++;
  if (newIndex < kCpuProfilerMaxThreads)
  {
    CpuThreadProfiler* threadProfiler = &_threadProfilers[newIndex];
    threadProfiler->Init(threadId, newIndex, threadName, maxTickTime_ms, logFreq, _creationTimePoint);
    
    PRINT_CH_INFO("CpuProfiler", "CpuProfiler.ThreadAdded", "Thread %u: '%s' added", threadProfiler->GetThreadIndex(), threadName);
    
    return threadProfiler;
  }
  else
  {
    // Add failed (no more room), undo the atomic increment
    --_threadProfilerCount;
    return nullptr;
  }
}

  
void CpuProfiler::RemoveThreadProfiler(CpuThreadId threadId)
{
  CpuThreadProfiler* existingProfiler = GetThreadProfiler(threadId);
  if (existingProfiler)
  {
    existingProfiler->SetHasBeenDeleted();
  }
}


// ================================================================================
// Console Functions for configuring profiling

#if REMOTE_CONSOLE_ENABLED  
static const char* kCpuProfilerSection = "CpuProfiler";

  
static CpuThreadProfiler* GetThreadByNameOrIndex(const char* threadName)
{
  CpuProfiler& cpuProfiler = CpuProfiler::GetInstance();
  CpuThreadProfiler* threadProfiler = cpuProfiler.GetThreadProfilerByName(threadName);
  if (!threadProfiler)
  {
    // See if threadName is an index instead
    int threadIndex = atoi(threadName);
    if ((threadIndex != 0) || (strcmp(threadName, "0") == 0))
    {
      threadProfiler = CpuProfiler::GetInstance().GetThreadProfilerByIndex(threadIndex);
    }
  }
  
  return threadProfiler;
}


static void SetThreadMaxTickTime( ConsoleFunctionContextRef context )
{
  context->channel->SetChannelName("CpuProfiler");
  
  const char* threadName      = ConsoleArg_Get_String( context, "threadName" );
  const double maxTickTime_ms = ConsoleArg_Get_Double( context, "maxTickTime_ms");
  
  CpuThreadProfiler* profiler = GetThreadByNameOrIndex(threadName);
  
  if (profiler)
  {
    context->channel->WriteLog(
                     "Changing Thread '%s' max tick time from %f to %f ms",
                     threadName, profiler->GetMaxTickTime_ms(), maxTickTime_ms);

    profiler->SetMaxTickTime_ms(maxTickTime_ms);
  }
  else
  {
    context->channel->WriteLog(
                        "Thread '%s' not found - cannot set tick time to %f ms",
                        threadName, maxTickTime_ms);
  }
}
CONSOLE_FUNC( SetThreadMaxTickTime, kCpuProfilerSection, const char* threadName, double maxTickTime_ms );

  
static void SetThreadLogFreq( ConsoleFunctionContextRef context )
{
  context->channel->SetChannelName("CpuProfiler");
  
  const char* threadName = ConsoleArg_Get_String( context, "threadName" );
  const uint32_t logFreq = ConsoleArg_Get_UInt32( context, "logFreq");
  
  CpuThreadProfiler* profiler = GetThreadByNameOrIndex(threadName);
  
  if (profiler)
  {
    context->channel->WriteLog("Changing Thread '%s' log frequency from %u to %u",
                               profiler->GetThreadName(), profiler->GetLogFrequency(), logFreq);
    
    profiler->SetLogFrequency(logFreq);
  }
  else
  {
    context->channel->WriteLog("Thread '%s' not found - cannot set log freq to %u", threadName, logFreq);
  }
}
CONSOLE_FUNC( SetThreadLogFreq, kCpuProfilerSection, const char* threadName, uint32_t logFreq );
  
  
static void LogProfile( ConsoleFunctionContextRef context )
{
  context->channel->SetChannelName("CpuProfiler");
  
  const char* threadName = ConsoleArg_Get_String( context, "threadName" );
  
  CpuThreadProfiler* profiler = GetThreadByNameOrIndex(threadName);
  
  if (profiler)
  {
    context->channel->WriteLog("Requesting Profile for thread '%s'", profiler->GetThreadName());
    profiler->RequestProfile();
  }
  else
  {
    context->channel->WriteLog("Thread '%s' not found - cannot show profile", threadName);
  }
}
CONSOLE_FUNC( LogProfile, kCpuProfilerSection, const char* threadName );

  
static void ListProfiledThreads( ConsoleFunctionContextRef context )
{
  context->channel->SetChannelName("CpuProfiler");
  
  const CpuProfiler& cpuProfiler = CpuProfiler::GetInstance();
  uint32_t profilerCount = cpuProfiler.GetThreadProfilerCount();
  for (uint32_t i = 0; i < profilerCount; ++i)
  {
    const CpuThreadProfiler* threadProfiler = cpuProfiler.GetThreadProfilerByIndex(i);
    if (threadProfiler)
    {
      context->channel->WriteLog(
                       "Thread %u: '%s', maxTick: %f, LogFreq: %u%s",
                       i, threadProfiler->GetThreadName(),
                       threadProfiler->GetMaxTickTime_ms(),
                       threadProfiler->GetLogFrequency(),
                       threadProfiler->HasBeenDeleted() ? " (DELETED)" : "");
    }

  }
}
CONSOLE_FUNC( ListProfiledThreads, kCpuProfilerSection );

#endif // REMOTE_CONSOLE_ENABLED
  
} // end namespace Util
} // end namespace Anki


#endif // ANKI_CPU_PROFILER_ENABLED

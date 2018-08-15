/**
 * File: timeProfiler.h
 *
 * Author: Kevin Yoon
 * Created: 07/13/2015
 *
 * Description:
 *
 *   Tool for doing time-profiling on robot code.
 *   
 * Usage:
 *   1) Start profiler.  e.g. START_TIME_PROFILE(MyProfiler, FIRST) where FIRST is the desired profile name
 *                            of the following block of code up until the next call to
 *                            MARK_NEXT_TIME_PROFILE or END_TIME_PROFILE.
 *
 *   2) Mark the end of the previous profile and the start of new one (Optional). 
 *                       e.g. MARK_NEXT_TIME_PROFILE(MyProfiler, SECOND)
 *
 *   3) Mark end of profiling  e.g. END_TIME_PROFILE(MyProfiler)
 *
 *      NB: All calls to START, MARK, and END should be in the same scope, or profiles may be corrupted!
 *
 *   4) Print stats to engine.  e.g. Call PRINT_TIME_PROFILE_STATS(MyProfiler)
 *      Don't do this on every main cycle tic or you may overload comms.
 *
 *   5) Reset profile to get fresh stats.  e.g. RESET_TIME_PROFILE(MyProfiler)
 *
 *
 * Copyright: Anki, Inc. 2015
 *
 **/

#include "coretech/common/shared/types.h"

namespace Anki {
namespace Vector {

// Uncomment to disable time profiling
//#define ENABLE_TIME_PROFILING
 
  
#ifdef ENABLE_TIME_PROFILING
  
#define START_TIME_PROFILE(profilerName, firstProfileName)  static TimeProfiler timeProfiler##profilerName(#profilerName); \
                                                            timeProfiler##profilerName.StartProfiling(#firstProfileName);
#define MARK_NEXT_TIME_PROFILE(profilerName, nextProfileName) timeProfiler##profilerName.MarkNextProfile(#nextProfileName);
#define END_TIME_PROFILE(profilerName)                      timeProfiler##profilerName.EndProfiling();
#define PRINT_TIME_PROFILE_STATS(profilerName)              timeProfiler##profilerName.PrintStats();
#define RESET_TIME_PROFILE(profilerName)                    timeProfiler##profilerName.Reset();
#define PERIODIC_PRINT_AND_RESET_TIME_PROFILE(profilerName, numCycles) static u32 cnt##profilerName = 0; \
                                                                       if (++cnt##profilerName >= numCycles) { \
                                                                         timeProfiler##profilerName.PrintStats(); \
                                                                         timeProfiler##profilerName.Reset(); \
                                                                         cnt##profilerName = 0; \
                                                                       }
  
  
#else
// if !defined(ENABLE_TIME_PROFILING)
#define START_TIME_PROFILE(profilerName, firstProfileName)
#define MARK_NEXT_TIME_PROFILE(profilerName, nextProfileName)
#define END_TIME_PROFILE(profilerName)
#define PRINT_TIME_PROFILE_STATS(profilerName)
#define RESET_TIME_PROFILE(profilerName)
#define PERIODIC_PRINT_AND_RESET_TIME_PROFILE(profilerName, numCycles)  
#endif
  
  class TimeProfiler {
  public:
    
    static const u32 MAX_NUM_PROFILES = 16;
    static const u32 MAX_PROF_NAME_LENGTH = 32;    
    
    TimeProfiler(const char* name);
    void Reset();

    void StartProfiling(const char* profName);
    void MarkNextProfile(const char* profName);
    void EndProfiling();
    
    const char* GetProfName(u32 index);
    u32 ComputeStats(u32 *avgTimes[], u32 *maxTimes[]);
    void PrintStats();
    
  private:
    char name_[MAX_PROF_NAME_LENGTH];    
    char timeProfName_[MAX_NUM_PROFILES][MAX_PROF_NAME_LENGTH];
                       
    u32 timeProfiles_[MAX_NUM_PROFILES];
    u32 timeProfIdx_;
    
    u32 totalTimeProfiles_[MAX_NUM_PROFILES];
    u32 numCyclesInProfile_;
    
    u32 maxTimeProfiles_[MAX_NUM_PROFILES];

    u32 avgTimeProfiles_[MAX_NUM_PROFILES];
    
    bool isProfiling_;

    
    void MarkNextProfile_Internal();
  };
  
} // namespace Vector
} // namespace Anki

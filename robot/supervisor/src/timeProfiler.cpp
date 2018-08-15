/**
 * File: timeProfiler.cpp
 *
 * Author: Kevin Yoon
 * Created: 07/13/2015
 *
 * Description:
 *
 *   Tool for doing time-profiling on robot code
 *
 * Copyright: Anki, Inc. 2015
 *
 **/


#include "timeProfiler.h"
#include "anki/cozmo/robot/hal.h"
#include "messages.h"
#include "clad/robotInterface/messageRobotToEngine_send_helper.h"
#include "anki/cozmo/robot/logging.h"
#include <stdio.h>
#include <string.h>

namespace Anki {
namespace Vector {

  TimeProfiler::TimeProfiler(const char* name)
  {
    strncpy(name_, name, MAX_PROF_NAME_LENGTH);
    name_[MAX_PROF_NAME_LENGTH-1] = 0;
    Reset();
  }

  void TimeProfiler::Reset()
  {
    numCyclesInProfile_ = 0;
    isProfiling_ = false;

    const u32 BUFSIZE = MAX_NUM_PROFILES * sizeof(u32);
    memset(timeProfiles_, 0, BUFSIZE);
    memset(totalTimeProfiles_, 0, BUFSIZE);
    memset(maxTimeProfiles_, 0, BUFSIZE);
  }

  void TimeProfiler::StartProfiling(const char* profName) {
    timeProfIdx_ = 0;
    timeProfiles_[timeProfIdx_] = HAL::GetMicroCounter();

    if (numCyclesInProfile_ == 0) {
      strncpy(timeProfName_[timeProfIdx_], profName, MAX_PROF_NAME_LENGTH);
      timeProfName_[timeProfIdx_][MAX_PROF_NAME_LENGTH-1] = 0;
    }

    ++timeProfIdx_;
    isProfiling_ = true;
  }

  void TimeProfiler::MarkNextProfile_Internal() {
    AnkiAssert(timeProfIdx_ < MAX_NUM_PROFILES, "");
    timeProfiles_[timeProfIdx_] = HAL::GetMicroCounter();

    // Update total time
    u32 duration = timeProfiles_[timeProfIdx_] - timeProfiles_[timeProfIdx_-1];
    totalTimeProfiles_[timeProfIdx_-1] += duration;

    // Update max observed time
    if (duration > maxTimeProfiles_[timeProfIdx_-1]) {
      maxTimeProfiles_[timeProfIdx_-1] = duration;
    }

    ++timeProfIdx_;
  }

  void TimeProfiler::MarkNextProfile(const char* profName) {
    AnkiAssert(timeProfIdx_ < MAX_NUM_PROFILES, "");

    if (numCyclesInProfile_ == 0) {
      strncpy(timeProfName_[timeProfIdx_], profName, MAX_PROF_NAME_LENGTH);
      timeProfName_[timeProfIdx_][MAX_PROF_NAME_LENGTH-1] = 0;
    }

    MarkNextProfile_Internal();
  }


  void TimeProfiler::EndProfiling() {
    MarkNextProfile_Internal();
    ++numCyclesInProfile_;
    isProfiling_ = false;
  }

  const char* TimeProfiler::GetProfName(u32 index)
  {
    AnkiConditionalWarnAndReturnValue((!isProfiling_), NULL, "timeprofiler.getprofname_while_busy", "GetProfName called in middle of profile. Ignoring.");

    if (index < timeProfIdx_) {
      return (const char*)&timeProfName_[index];
    }

    return NULL;
  }

  u32 TimeProfiler::ComputeStats(u32 *avgTimes[], u32 *maxTimes[]) {
    AnkiConditionalWarnAndReturnValue((!isProfiling_), 0, "timeprofiler.computestats_while_busy", "ComputeStats called in middle of profile. Ignoring.");

    // Compute average time
    for(u8 i=0; i<timeProfIdx_; ++i) {
      avgTimeProfiles_[i] = totalTimeProfiles_[i] / numCyclesInProfile_;
    }

    if (avgTimes)  *avgTimes = avgTimeProfiles_;
    if (maxTimes)  *maxTimes = maxTimeProfiles_;

    return timeProfIdx_-1;
  }

  void TimeProfiler::PrintStats() {
    u32 *avgTimes;
    u32 *maxTimes;

    u32 numProfiles = ComputeStats(&avgTimes, &maxTimes);

    for (u32 i=0; i<numProfiles; ++i) {
      AnkiInfo("TimeProfiler.PrintStats", 
               "Profile: %s - %s, avgTime: %d, maxFime: %d",
               name_, timeProfName_[i], avgTimes[i], maxTimes[i]);
    }
  }



} // namespace Vector
} // namespace Anki

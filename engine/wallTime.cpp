/**
 * File: wallTime.cpp
 *
 * Author: Brad Neuman
 * Created: 2018-06-11
 *
 * Description: Utilities for getting wall time
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#include "engine/wallTime.h"

#include "coretech/common/engine/utils/timer.h"
#include "osState/osState.h"
#include "util/logging/logging.h"

#include <ctime>
#include <chrono>

namespace Anki {
namespace Cozmo {

namespace {

// while we think we're synced, re-check every so often
static const float kSyncCheckPeriodWhenSynced_s = 60.0f * 60.0f;

// check more often if we aren't synced (so we get the accurate time after a sync)
static const float kSyncCheckPeriodWhenNotSynced_s = 1.0f;

}

WallTime::WallTime()
{
}

WallTime::~WallTime()
{
}

bool WallTime::GetLocalTime(struct tm& localTime)
{
  if( !IsTimeSynced() ) {
    return false;
  }

  const bool timeOK = WallTime::GetApproximateLocalTime(localTime);
  return timeOK;
}

bool WallTime::GetUTCTime(struct tm& utcTime)
{
  if( !IsTimeSynced() ) {
    return false;
  }

  const bool timeOK = WallTime::GetApproximateUTCTime(utcTime);
  return timeOK;
}

bool WallTime::GetApproximateUTCTime(struct tm& utcTime)
{
  using namespace std::chrono;
  const time_t now = system_clock::to_time_t(system_clock::now());
  tm* utc = gmtime(&now);
  if( nullptr != utc ) {
    utcTime = *utc;
    return true;
  }

  PRINT_NAMED_ERROR("WallTime.UTC.Invalid",
                    "gmtime returned null. Error: %s",
                    strerror(errno));
  return false;
}

bool WallTime::GetApproximateLocalTime(struct tm& localTime)
{  
  using namespace std::chrono;
  const time_t now = system_clock::to_time_t(system_clock::now());
  tm* local = localtime(&now);
  if( nullptr != local ) {
    localTime = *local;
    return true;
  }

  PRINT_NAMED_ERROR("WallTime.Local.Invalid",
                    "localtime returned null. Erro`r: %s",
                    strerror(errno));
  return false;
}

bool WallTime::IsTimeSynced()
{
  // Use base station timer because it's cheap and good enough here, goal is just to not hit the syscall too
  // frequently if this function is called often
  const float currTime_s = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();

  const float checkPeriod_s = _wasSynced ? kSyncCheckPeriodWhenSynced_s : kSyncCheckPeriodWhenNotSynced_s;
  if( _lastSyncCheckTime < 0 ||
      _lastSyncCheckTime + checkPeriod_s <= currTime_s ) {
    _wasSynced = OSState::getInstance()->IsWallTimeSynced();
    _lastSyncCheckTime = currTime_s;
  }

  return _wasSynced;
}

}
}

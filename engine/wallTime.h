/**
 * File: wallTime.h
 *
 * Author: Brad Neuman
 * Created: 2018-06-11
 *
 * Description: Utilities for getting wall time
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#ifndef __Engine_WallTime_H__
#define __Engine_WallTime_H__

#include "util/singleton/dynamicSingleton.h"

// forward declare time struct (see <ctime>)
struct tm;

namespace Anki {
namespace Cozmo {

class WallTime : public Util::DynamicSingleton<WallTime>
{
  ANKIUTIL_FRIEND_SINGLETON(WallTime);

public:
  ~WallTime();

  // NOTE: None of these timers are monotonic or steady. They are all based on system time which can be set via
  // NTP or changed (potentially by the user)

  // If time is synchronized (reasonably accurate), fill the passed in unix time struct with the current local
  // time and return true. Otherwise, return false. Note that if a timezone is not set (see
  // OSState::HasTimezone()), UTC is the default on vic-os
  bool GetLocalTime(struct tm& localTime);

  // If the time is synchronized (reasonably accurate), fill the passed in unix time struct with the current
  // time in UTC and return true. Otherwise, return false.
  bool GetUTCTime(struct tm& utcTime);

  // If the time is _not_ synchronized since boot (e.g. we aren't on wifi) and/or we don't know the timezone,
  // we can still get an approximate UTC time. Note that this may be arbitrarily behind the real time, e.g. if
  // the robot has been off wifi (or the NTP servers are down for some reason) for a year, this time may be a
  // year behind. Returns false if there's an internal error, true if it set the time
  bool GetApproximateUTCTime(struct tm& utcTime);

  // Set the approximate local time regardless of synchronization and return true (false if error). Note that
  // similat to GetLocalTime(), vicos will default to UTC if no timezone is set (see OSState::HasTimezone())
  bool GetApproximateLocalTime(struct tm& localTime);

private:

  WallTime();

  bool IsTimeSynced();
  
  // checking for time sync is a syscall, so avoid doing it too often by keeping a cache and refreshing based
  // on a different timer
  float _lastSyncCheckTime = -1.0f;
  bool _wasSynced = false;

};

}
}


#endif

/**
 * File: timer.h
 *
 * Author: bsofman (boris)
 * Created: 1/6/2009
 *
 * Updated by: damjan
 * Updated on: 9/21/2012
 *
 * Description:
 * BaseStationTimer - Keep track of system time. Provides easy way to get time since the start of program.
 * SchedulingTimer - A timer class that can be used to measure periods of real time repeatedly.
 *
 * Copyright: Anki, Inc. 2011
 *
 **/


#ifndef BASESTATION_UTILS_TIMER_H_
#define BASESTATION_UTILS_TIMER_H_

//#include <queue>
//#include <unistd.h>
#include <string>
#include <time.h>
#include <sys/time.h>

#include "anki/common/types.h"
#include "anki/util/logging/iTickTimeProvider.h"


namespace Anki {

// Returns current time in milliseconds (used for debug when BaseStation is not running)
int getTimeInMilliSeconds();
// Returns current time in seconds (used for debug when BaseStation is not running)
float getTimeInSeconds();
  
  
// Get current date/time, format is YYYY-MM-DD_HH-mm-ss
const std::string GetCurrentDateTime();
  
  
/*
 * Keep track of system time. Provides easy way to get time since the start of program.
 */
class BaseStationTimer : public Anki::Util::ITickTimeProvider
{
  public:

  // Method to fetch singleton instance.
  static BaseStationTimer* getInstance();

  //Removes instance
  static void removeInstance();

  // Destructor
  virtual ~BaseStationTimer() override;

  // Gets time in seconds since the start of the program
  // WARNING: This value updates only once every tick! So measuring time differneces with in one update loop
  // will result in ZERO time passed.
  double GetCurrentTimeInSeconds() const;

  // Gets time in nanoseconds since the start of the program
  // WARNING: This value updates only once every tick! So measuring time differneces with in one update loop
  // will result in ZERO time passed.
  BaseStationTime_t GetCurrentTimeInNanoSeconds() const;

  // Gets elapsed time since last tick in seconds
  double GetTimeSinceLastTickInSeconds() const;

  // Gets elapsed time since last tick in nanoseconds
  BaseStationTime_t GetTimeSinceLastTickInNanoSeconds() const;

  // Updates the current system time used for tracking
  void UpdateTime(BaseStationTime_t currTime);
  
  // Gets current time in TimeStamp units
  TimeStamp_t GetCurrentTimeStamp() const;
  
  virtual const size_t GetTickCount() const override;
  virtual const float GetRunTime() const override;
  
private:
  // Constructor
  BaseStationTimer();

  // Private static member referencing the single instance of the object
  static BaseStationTimer* _instance;

  // Current system time used for tracking in all timer instances
  double currentTimeInSeconds_;
  BaseStationTime_t currentTimeInNanoSeconds_;

  double elapsedTimeInSeconds_;
  BaseStationTime_t elapsedTimeInNanoSeconds_;
  
  size_t tickCount_;
};

  /* Not used by Cozmo
   
// A timer class that can be used to measure periods of real time repeatedly.
// After it is initialized, calls to IsReady() will return whether the necessary
// period has elapsed while simultaneously resetting the time target for the
// next cycle (if repeating is true).  If repeating is enabled, the next time
// target is not the period amount from the current time but rather a multiple
// of periodTime off of the initialization time.
//
// Also allows the system to sleep until the next scheduled trigger time so that
// we don't have to use up the CPU.
class SchedulingTimer {

public:

  // Constructors
  SchedulingTimer() { periodTime_ = -1; } // Not valid
  SchedulingTimer(double periodTime, bool repeating, bool checkForPeriodSkips)
  {
    periodTime_ = periodTime;
    lastFireTime_ = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
    repeating_ = repeating;
    checkForPeriodSkips_ = checkForPeriodSkips;
    firedFirstTime_ = false;
  }

  // If no arguments: resets this SchedulingTimer object's current cycle.
  //
  // If arguments:
  //   newPeriodTime: changes timer period
  //   repeating:     changes repeating state
  //   checkForPeriodSkips: Whether to print a warning if more than one period
  //                        has elapsed between calls on a repeating timer.
  void Reset() { lastFireTime_ = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();}
  void Reset(double newPeriodTime, bool repeating, bool checkForPeriodSkips = true);

  // Returns true if the specified amount of time has elapsed since
  // initialization.  If so, adjusts the target time for the next period.
  bool IsReady();

  double GetPeriod() const {return periodTime_;}


private:

  // Last time this timer object fired
  double lastFireTime_;

  // SchedulingTimer period
  double periodTime_;

  // Whether this timer resets on each IsReady() that returns true or not (if
  // not resetting, it will always return true once it returns true once until
  // it is reset).
  bool repeating_;

  // Whether to print a warning if more than one period has elapsed between
  // calls on a repeating timer.
  bool checkForPeriodSkips_;

  // Wheter the timer fired for the first time (to eliminate timing error
  // message first cycle)
  bool firedFirstTime_;

};
*/
  
} // namespace Anki

#endif // BASESTATION_UTILS_TIMER_H_

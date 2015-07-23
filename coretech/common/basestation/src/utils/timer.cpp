/**
 * File: timer.cpp
 *
 * Author: bsofman (boris)
 * Created: 2/1/2009
 *
 * Updated by: damjan
 * Updated on: 9/21/2012
 *
 * Description:
 * BaseStationTimer - Keep track of system time. Provides easy way to get time since the start of program.
 * SchedulingTimer - A timer class that can be used to measure periods of real time repeatedly.
 *
 * Copyright: Anki, Inc. 2008-2011
 *
 **/

#include "anki/common/basestation/utils/timer.h"
//#include "basestation/utils/parameters.h"

#ifndef LINUX
#include <mach/mach_time.h>
#endif

namespace Anki {

  /*
int getTimeInMilliSeconds()
{
  #ifndef LINUX
  {
    const int64_t kOneMillion = 1000 * 1000;
    static mach_timebase_info_data_t s_timebase_info;

    if (s_timebase_info.denom == 0) {
      (void) mach_timebase_info(&s_timebase_info);
    }

    // mach_absolute_time() returns billionth of seconds,
    // so divide by one million to get milliseconds
    return (int)((mach_absolute_time() * s_timebase_info.numer) / (kOneMillion * s_timebase_info.denom));
  }
  #else
  {
    return 0;
  }
  #endif
}
 
float getTimeInSeconds()
{
  const float secs = getTimeInMilliSeconds() / 1000.0f;
  return secs;
}
  */
  
  

const std::string GetCurrentDateTime() {
  time_t     now = time(0);
  struct tm  tstruct;
  char       buf[80];
  tstruct = *localtime(&now);
  strftime(buf, sizeof(buf), "%F_%H-%M-%S", &tstruct);
  
  return buf;
}
  
  
  
// Definition of static field
BaseStationTimer* BaseStationTimer::_instance = 0;

/**
 * Returns the single instance of the object.
 */
BaseStationTimer* BaseStationTimer::getInstance() {
  // check if the instance has been created yet
  if(0 == _instance) {
    // if not, then create it
    _instance = new BaseStationTimer;
  }
  // return the single instance
  return _instance;
}

/**
* Removes instance
*/
void BaseStationTimer::removeInstance() {
  // check if the instance has been created yet
  if(0 != _instance) {
    delete _instance;
    _instance = 0;
  }
};

// Constructor implementation
BaseStationTimer::BaseStationTimer() :
  currentTimeInSeconds_(0.0),
  currentTimeInNanoSeconds_(0),
  elapsedTimeInSeconds_(0.0),
  elapsedTimeInNanoSeconds_(0)
{}

// Destructor implementation
BaseStationTimer::~BaseStationTimer()
{

}


// Updates the current system time used for tracking
void BaseStationTimer::UpdateTime(BaseStationTime_t currTime)
{
  elapsedTimeInNanoSeconds_ = currTime - currentTimeInNanoSeconds_;
  elapsedTimeInSeconds_ = NANOS_TO_SEC(elapsedTimeInNanoSeconds_);

  currentTimeInNanoSeconds_ = currTime;
  currentTimeInSeconds_ = NANOS_TO_SEC(currTime);
  
  //printf("Updating basestation time to %llu\n", currentTimeInNanoSeconds_);
  tickCount_ += 1;
}


//static int localCount = 0;
double BaseStationTimer::GetCurrentTimeInSeconds() const
{
  return currentTimeInSeconds_;
}

BaseStationTime_t BaseStationTimer::GetCurrentTimeInNanoSeconds() const
{
  return currentTimeInNanoSeconds_;
}

double BaseStationTimer::GetTimeSinceLastTickInSeconds() const
{
  return elapsedTimeInSeconds_;
}

BaseStationTime_t BaseStationTimer::GetTimeSinceLastTickInNanoSeconds() const
{
  return elapsedTimeInNanoSeconds_;
}

TimeStamp_t BaseStationTimer::GetCurrentTimeStamp() const
{
  return static_cast<TimeStamp_t>(currentTimeInNanoSeconds_ / 1000000);
}
  
const size_t BaseStationTimer::GetTickCount() const
{
  return tickCount_;
}

const float BaseStationTimer::GetRunTime() const
{
  return static_cast<float>(currentTimeInSeconds_);
}
  
  /*
void SchedulingTimer::Reset(double newPeriodTime, bool repeating, bool checkForPeriodSkips)
{
  if(newPeriodTime > 0) {
    periodTime_ = newPeriodTime;
  }
  lastFireTime_ = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
  repeating_ = repeating;
  checkForPeriodSkips_ = checkForPeriodSkips;
  firedFirstTime_ = false;

}


bool SchedulingTimer::IsReady()
{
  // Not initialized
  if(periodTime_ < 0) {
    //PRINT_NAMED_WARNING("SchedulingTimer", "Attempting to use uninitialized timer.");
    return false;
  }

  // Skip logic if this is a non-repeating timer that already fired
  if(!repeating_ && firedFirstTime_) {
    //PRINT_NAMED_ERROR("SchedulingTimer", "Non-repeating timer already fired. Won't fire again...");
    return false;
  }

  // Check for sufficiently elapsed time
  bool ready = false;
  bool printedError = false;
  while(lastFireTime_ + periodTime_ <= BaseStationTimer::getInstance()->GetCurrentTimeInSeconds() + FLOATING_POINT_COMPARISON_TOLERANCE) {

    // If repeating mode, setup for next iteration
    if(repeating_) {
      lastFireTime_ += periodTime_;

      // If runs more than once this means we skipped over a period
      if(checkForPeriodSkips_ && ready && !printedError && firedFirstTime_) {
        printedError = true;
        PRINT_NAMED_WARNING("SchedulingTimer", "Skipped over a period of timer.  Elapsed time: %f, Period = %f",
            BaseStationTimer::getInstance()->GetCurrentTimeInSeconds() - lastFireTime_, periodTime_);
      }

      ready = true;
    } else {
      // Simply need to return here since we're not adjusting for next cycle
      ready = true;
      break;
    }

  }

  if(ready) {
    firedFirstTime_ = true;
  }

  return ready;
}
*/



} // namespace Anki


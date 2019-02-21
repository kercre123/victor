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

#include "coretech/common/engine/utils/timer.h"
#include "util/helpers/templateHelpers.h"
#include "util/math/math.h"
#include "util/math/numericCast.h"


namespace Anki {
  
// Definition of static field
BaseStationTimer* BaseStationTimer::_instance = 0;

/**
 * Returns the single instance of the object.
 */
BaseStationTimer* BaseStationTimer::getInstance()
{
  if (nullptr == _instance) {
     _instance = new BaseStationTimer;
  }
  return _instance;
}

/**
* Removes instance
*/
void BaseStationTimer::removeInstance()
{
  Util::SafeDelete(_instance);
}

BaseStationTimer::BaseStationTimer()
: currTimeSecondsDouble_(0.0)
, currTimeSecondsFloat_(0.0f)
, currTimeNanoSeconds_(0)
, currTimeStamp_(0)
, tickCount_(0)
{
}

BaseStationTimer::~BaseStationTimer()
{
}


// Updates the current system time used for tracking

void BaseStationTimer::UpdateTime(const BaseStationTime_t currTime_NS)
{
  // Store the current time in different convenient formats
  currTimeNanoSeconds_ = currTime_NS;
  currTimeSecondsDouble_ = Util::numeric_cast<double>(Util::NanoSecToSec(currTime_NS));
  currTimeSecondsFloat_ = Util::numeric_cast<float>(currTimeSecondsDouble_);
  currTimeStamp_ = Util::numeric_cast<TimeStamp_t>(Util::SecToMilliSec(currTimeSecondsDouble_));

  ++tickCount_;
}


float BaseStationTimer::GetCurrentTimeInSeconds() const
{
  return currTimeSecondsFloat_;
}

double BaseStationTimer::GetCurrentTimeInSecondsDouble() const
{
  return currTimeSecondsDouble_;
}

BaseStationTime_t BaseStationTimer::GetCurrentTimeInNanoSeconds() const
{
  return currTimeNanoSeconds_;
}

TimeStamp_t BaseStationTimer::GetCurrentTimeStamp() const
{
  return currTimeStamp_;
}

const size_t BaseStationTimer::GetTickCount() const
{
  return tickCount_;
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


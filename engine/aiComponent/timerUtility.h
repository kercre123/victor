/**
* File: timerUtility.h
*
* Author: Kevin M. Karol
* Created: 2/5/18
*
* Description: Keep track of information relating to the user facing
* timer utility
*
* Copyright: Anki, Inc. 2018
*
**/

#ifndef __Cozmo_Basestation_BehaviorSystem_TimerUtility_H__
#define __Cozmo_Basestation_BehaviorSystem_TimerUtility_H__

#include "util/entityComponent/iManageableComponent.h"
#include "util/helpers/noncopyable.h"
#include "util/logging/logging.h"

#include <memory>

namespace Anki {
namespace Cozmo {

class Robot;
  
class TimerHandle{
  public:
    TimerHandle(int totalTime_s)
    : _timerLength(totalTime_s)
    , _endTime_s(GetSystemTime_s() + totalTime_s){}

    static const int kSecInHour = 3600;

    static int SecondsToDisplayHours(int seconds)   { return seconds /kSecInHour;}
    static int SecondsToDisplayMinutes(int seconds) { return (seconds % kSecInHour)/60;}
    static int SecondsToDisplaySeconds(int seconds) { return seconds % 60; }

    // Get time and easy accessors
    int GetTimerLength_s()        const { return _timerLength; }
    // access the "displayable" value for each time unit in the total timer
    int GetDisplayHoursLength()   const { return SecondsToDisplayHours(_timerLength);   }
    int GetDisplayMinutesLength() const { return SecondsToDisplayMinutes(_timerLength); }
    int GetDisplaySecondsLength() const { return SecondsToDisplaySeconds(_timerLength); }
    
    // Return the full time remaining in seconds
    int GetTimeRemaining_s() const {   
      const int timeRemaining = _endTime_s - GetSystemTime_s();
      return  timeRemaining >= 0 ? timeRemaining : 0;
    }
    // access the "displayable" value for each time unit remaining
    int GetDisplayHoursRemaining()   const { return SecondsToDisplayHours(GetTimeRemaining_s());}
    int GetDisplayMinutesRemaining() const { return SecondsToDisplayMinutes(GetTimeRemaining_s());}
    int GetDisplaySecondsRemaining() const { return SecondsToDisplaySeconds(GetTimeRemaining_s());}
  private:
    // helper for easy access to current time
    static int GetSystemTime_s();

    const int _timerLength;
    const int _endTime_s;
};

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// TimerUtility
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
class TimerUtility : public IManageableComponent , private Util::noncopyable
{
public:
  using SharedHandle = std::shared_ptr<TimerHandle>;

  // constructor
  TimerUtility();
  virtual ~TimerUtility();

  SharedHandle StartTimer(int timerLength_s)
  {
    ANKI_VERIFY(_activeTimer->GetTimeRemaining_s() == 0,
                "TimerUtility.StartTimer.TimerAlreadySet", 
                "Current design says we don't overwrite timers - remove this verify if that changes");
    _activeTimer = std::make_shared<TimerHandle>(timerLength_s);
    return _activeTimer;
  }
  SharedHandle GetActiveTimer() const { return _activeTimer; }

  int GetSystemTime_s() const;

private:
  SharedHandle _activeTimer;

};

} // namespace Cozmo
} // namespace Anki

#endif // __Cozmo_Basestation_BehaviorSystem_TimerUtility_H__

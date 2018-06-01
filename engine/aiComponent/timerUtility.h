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

#include "coretech/common/shared/types.h"
#include "engine/aiComponent/aiComponents_fwd.h"
#include "util/entityComponent/iDependencyManagedComponent.h"
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


    #if ANKI_DEV_CHEATS
    // "Advance" time by shortening the time remaining
    void AdvanceTimeBySeconds(u32 secondsToAdvance){_endTime_s -= secondsToAdvance;}
    #endif

  private:
    // helper for easy access to current time
    static int GetSystemTime_s();

    const int _timerLength;
    int _endTime_s;
};

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// TimerUtility
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
class TimerUtility : public IDependencyManagedComponent<AIComponentID>, 
                     private Util::noncopyable
{
public:
  using SharedHandle = std::shared_ptr<TimerHandle>;

  // constructor
  TimerUtility();
  virtual ~TimerUtility();

  SharedHandle GetTimerHandle() const { return _activeTimer; }
  SharedHandle StartTimer(int timerLength_s);
  void ClearTimer();

  int GetSystemTime_s() const;

    #if ANKI_DEV_CHEATS
    // "Advance" time by shortening the time remaining
    void AdvanceTimeBySeconds(u32 secondsToAdvance){ if(_activeTimer != nullptr){_activeTimer->AdvanceTimeBySeconds(secondsToAdvance); }}
    #endif

private:
  SharedHandle _activeTimer;
  
};

} // namespace Cozmo
} // namespace Anki

#endif // __Cozmo_Basestation_BehaviorSystem_TimerUtility_H__

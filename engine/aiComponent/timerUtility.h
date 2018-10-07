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
#include "engine/ankiEventUtil.h"

#include <memory>
#include <list>

namespace Anki {
namespace Vector {


class Robot;
namespace RobotInterface {
  class RobotToEngine;
}
  
class TimerHandle{
  public:
    TimerHandle(int totalTime_s)
    : _timerLength(totalTime_s)
    , _endTime_s(GetSteadyTime_s() + totalTime_s){}

    static const int kSecInHour = 3600;
  
  void SetTimeSinceEpoch(int time) { _endTime_s = time; }

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
      const int timeRemaining = _endTime_s - GetSteadyTime_s();
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

    void SetAlexaID(int alexaID){ _alexaID = alexaID; }
    int GetAlexaID() const { return _alexaID; }
  
  void SetIsAlarm(bool isAlarm){ _isAlarm = isAlarm; }
  bool GetIsAlarm() const { return _isAlarm; }
  
  
  
  private:
    // helper for easy access to current time
    static int GetSteadyTime_s();

    const int _timerLength;
    int _endTime_s;
  
    int _alexaID=0;
    bool _isAlarm = false;
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
  explicit TimerUtility(const Robot& robot);
  virtual ~TimerUtility();

  SharedHandle GetSoonestTimer() const;
  //SharedHandle GetTimerHandle() const { return _activeTimer; }
  SharedHandle StartTimer(int timerLength_s); // starts a vector timer
  void ClearRingingTimers(); // clears those that elapsed
  void ClearVectorTimer(); // clear vector's timer
  void ClearAllTimers();

  int GetSteadyTime_s() const;
  int GetEpochTime_s() const;

#if ANKI_DEV_CHEATS
  // "Advance" time by shortening the time remaining
  void AdvanceTimeBySeconds(u32 secondsToAdvance){
    auto vt = GetVectorTimer();
    if( vt != nullptr){
      vt->AdvanceTimeBySeconds(secondsToAdvance);
      
    }
  }
#endif
  SharedHandle GetVectorTimer() const { return *_allTimers.begin(); };
  
  bool HasPendingCancel(int* remaining_s = nullptr) const;
  void ResetPendingCancel() { _cancelTime = 0; }
  
  bool HasPendingSet(int* remaining_s = nullptr) const;
  void ResetPendingSet() { _setTime = 0; }

private:
  void OnAlexaAlerts( const AnkiEvent<RobotInterface::RobotToEngine>& message );
  void NotifyAlexaOfClearedTimers(const std::vector<int>& timers);
  // first in list is always vector's one timer
  std::list<SharedHandle> _allTimers;
  
  std::list<Signal::SmartHandle> _handles;

  const Robot* _robot;
  int _cancelTime = 0;
  int _setTime = 0;
};

} // namespace Vector
} // namespace Anki

#endif // __Cozmo_Basestation_BehaviorSystem_TimerUtility_H__

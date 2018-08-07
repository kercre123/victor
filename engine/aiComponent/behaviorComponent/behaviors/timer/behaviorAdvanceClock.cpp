/**
 * File: BehaviorAdvanceClock.cpp
 *
 * Author: Kevin M. Karol
 * Created: 2018-05-30
 *
 * Description: Advance the clock display between two times over the specified interval
 *
 * Copyright: Anki, Inc. 2018
 *
 **/


#include "engine/aiComponent/behaviorComponent/behaviors/timer/behaviorAdvanceClock.h"

#include "engine/aiComponent/timerUtility.h"

namespace Anki {
namespace Vector {
  

namespace{
const char* kStartTimeSecKey         = "startTime_sec";
const char* kEndTimeSecKey           = "endTime_sec";
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorAdvanceClock::BehaviorAdvanceClock(const Json::Value& config)
: BehaviorProceduralClock(config)
{
  const std::string kDebugStr = "BehaviorAdvanceClock.ParsingIssue";
  if(config.isMember(kStartTimeSecKey)){
    _startTime_sec = JsonTools::ParseInt32(config, kStartTimeSecKey, kDebugStr);
  }
  if(config.isMember(kEndTimeSecKey)){
    _endTime_sec   = JsonTools::ParseInt32(config, kEndTimeSecKey, kDebugStr);
  }

  SetGetDigitFunction(BuildTimerFunction());
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorAdvanceClock::~BehaviorAdvanceClock()
{
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorAdvanceClock::GetBehaviorJsonKeysInternal(std::set<const char*>& expectedKeys) const 
{
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorAdvanceClock::SetAdvanceClockParams(int startTime_sec, int endTime_sec, int displayTime_sec)
{
  _startTime_sec = startTime_sec;
  _endTime_sec   = endTime_sec;
  SetTimeDisplayClock_sec(displayTime_sec);
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorAdvanceClock::TransitionToShowClockInternal()
{

  for(int i = 0; i <= GetTotalNumberOfUpdates(); i++){
    BuildAndDisplayProceduralClock(i, i*ANIM_TIME_STEP_MS);   
  }
}



// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorProceduralClock::GetDigitsFunction BehaviorAdvanceClock::BuildTimerFunction() const
{
  return [this](const int offset){
    const int secToAdvance = GetTotalSecToAdvance();
    const int totalUpdates = GetTotalNumberOfUpdates();
    const int currentSecondsOffset = (secToAdvance * offset)/totalUpdates;
    const int nextSecToDisplay = TimeGoesUp() ? _startTime_sec + currentSecondsOffset
                                              : _startTime_sec - currentSecondsOffset;

    std::map<Vision::SpriteBoxName, int> outMap;
    
    const int hoursRemaining = TimerHandle::SecondsToDisplayHours(nextSecToDisplay);
    const int minsRemaining = TimerHandle::SecondsToDisplayMinutes(nextSecToDisplay);
    const int secsRemaining = TimerHandle::SecondsToDisplaySeconds(nextSecToDisplay);

    const bool displayingHoursOnScreen = (hoursRemaining > 0);

    // Tens Digit (left of colon)
    {
      int tensDigit = 0;
      if(displayingHoursOnScreen){
        tensDigit =  hoursRemaining/10;
      }else{
        tensDigit = minsRemaining/10;
      }
      outMap.emplace(std::make_pair(Vision::SpriteBoxName::TensLeftOfColon, tensDigit));
    }
    
    // Ones Digit (left of colon)
    {
      int onesDigit = 0;
      if(displayingHoursOnScreen){
        onesDigit = hoursRemaining % 10;
      }else{
        onesDigit = minsRemaining % 10;
      }
      outMap.emplace(std::make_pair(Vision::SpriteBoxName::OnesLeftOfColon, onesDigit));
    }

    // Tens Digit (right of colon)
    {
      int tensDigit = 0;
      if(displayingHoursOnScreen){
        tensDigit = minsRemaining/10;
      }else{
        tensDigit = secsRemaining/10;
      }
      outMap.emplace(std::make_pair(Vision::SpriteBoxName::TensRightOfColon, tensDigit));
    }

    // Ones Digit (right of colon)
    {
      int onesDigit = 0;
      if(displayingHoursOnScreen){
        onesDigit = minsRemaining % 10;
      }else{
        onesDigit = secsRemaining % 10;
      }
      outMap.emplace(std::make_pair(Vision::SpriteBoxName::OnesRightOfColon, onesDigit));
    }

    return outMap;
  };
}


}
}

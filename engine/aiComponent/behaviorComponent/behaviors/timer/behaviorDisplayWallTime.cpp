/**
 * File: BehaviorDisplayWallTime.cpp
 *
 * Author: Kevin M. Karol
 * Created: 2018-05-30
 *
 * Description: If the robot has a valid wall time, display it on the robot's face
 *
 * Copyright: Anki, Inc. 2018
 *
 **/


#include "engine/aiComponent/behaviorComponent/behaviors/timer/behaviorDisplayWallTime.h"

#include "engine/aiComponent/timerUtility.h"
#include "engine/components/settingsManager.h"
#include "engine/wallTime.h"

#include "util/console/consoleInterface.h"

namespace Anki {
namespace Vector {
  

namespace{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorDisplayWallTime::BehaviorDisplayWallTime(const Json::Value& config)
: BehaviorProceduralClock(config)
{
  SetGetDigitFunction(BuildTimerFunction());
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorDisplayWallTime::~BehaviorDisplayWallTime()
{
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorDisplayWallTime::GetBehaviorJsonKeysInternal(std::set<const char*>& expectedKeys) const 
{
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorDisplayWallTime::TransitionToShowClockInternal()
{
  BuildAndDisplayProceduralClock(0, 0);
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorDisplayWallTime::WantsToBeActivatedBehavior() const
{
  // Ensure we can get local time
  struct tm unused;
  return WallTime::getInstance()->GetLocalTime(unused);
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorProceduralClock::GetDigitsFunction BehaviorDisplayWallTime::BuildTimerFunction() const
{
  return [this](const int offset){
    std::map<Vision::SpriteBoxName, int> outMap;
    struct tm localTime;
    if(!WallTime::getInstance()->GetLocalTime(localTime)){
      return outMap;
    }
    
    // Convert to seconds
    const int currentMins = localTime.tm_min;
    int currentHours = localTime.tm_hour;

    if(!ShouldDisplayAsMilitaryTime()){
      currentHours = currentHours % 12;
    }

    // Tens Digit (left of colon)
    {
      const int tensDigit = currentHours/10;
      outMap.emplace(std::make_pair(Vision::SpriteBoxName::TensLeftOfColon, tensDigit));
    }
    
    // Ones Digit (left of colon)
    {
      const int onesDigit = currentHours % 10;
      outMap.emplace(std::make_pair(Vision::SpriteBoxName::OnesLeftOfColon, onesDigit));
    }

    // Tens Digit (right of colon)
    {
      const int tensDigit = currentMins/10;
      outMap.emplace(std::make_pair(Vision::SpriteBoxName::TensRightOfColon, tensDigit));
    }

    // Ones Digit (right of colon)
    {
      const int onesDigit = currentMins % 10;
      outMap.emplace(std::make_pair(Vision::SpriteBoxName::OnesRightOfColon, onesDigit));
    }

    return outMap;
  };
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorDisplayWallTime::ShouldDisplayAsMilitaryTime() const
{
  const auto& settingsManager = GetBEI().GetSettingsManager();
  const bool clockIs24Hour = settingsManager.GetRobotSettingAsBool(RobotSetting::clock_24_hour);
  return clockIs24Hour;
}

} // namespace Vector
} // namespace Anki

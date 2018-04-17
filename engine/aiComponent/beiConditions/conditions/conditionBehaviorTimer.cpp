/**
* File: conditionBehaviorTimer.cpp
*
* Author: ross
* Created: feb 23 2018
*
* Description: Condition that checks the state of a shared named BehaviorTimer
*
* Copyright: Anki, Inc. 2018
*
**/


#include "engine/aiComponent/beiConditions/conditions/conditionBehaviorTimer.h"
#include "engine/aiComponent/behaviorComponent/behaviorTimers.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/behaviorExternalInterface.h"

#include "coretech/common/engine/jsonTools.h"

#include "util/console/consoleInterface.h"

namespace Anki {
namespace Cozmo {
  
CONSOLE_VAR_EXTERN(float, kTimeMultiplier);

namespace{
const char* kTimerNameKey = "timerName";
const char* kCooldownKey = "cooldown_s";
const char* kParseDebug = "ConditionBehaviorTimer";
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ConditionBehaviorTimer::ConditionBehaviorTimer(const Json::Value& config, BEIConditionFactory& factory)
  : IBEICondition(config, factory)
{
  _timerName = JsonTools::ParseString(config, kTimerNameKey, kParseDebug);
  _cooldown_s = JsonTools::ParseFloat(config, kCooldownKey, kParseDebug);
  
  ANKI_VERIFY( BehaviorTimerManager::IsValidName( _timerName ),
               "ConditionBehaviorTimer.ctor.InvalidName",
               "Behavior timer '%s' is not valid",
               _timerName.c_str() );
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ConditionBehaviorTimer::~ConditionBehaviorTimer()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool ConditionBehaviorTimer::AreConditionsMetInternal(BehaviorExternalInterface& behaviorExternalInterface) const
{
  const auto timerType = BehaviorTimerManager::BehaviorTimerFromString( _timerName );
  const auto& timer = behaviorExternalInterface.GetBehaviorTimerManager().GetTimer( timerType );
  const bool valueIfNoReset = true;
  const bool expired = timer.HasCooldownExpired( _cooldown_s / kTimeMultiplier, valueIfNoReset );
  return expired;
}

} // namespace
} // namespace

/**
* File: strategyRobotTouched.cpp
*
* Author: Arjun Menon
* Created: 10/18/2017
*
* Description: 
* Strategy that wants to run when the robot is being touched
*
* Copyright: Anki, Inc. 2017
*
**/

#include "engine/aiComponent/beiConditions/conditions/conditionRobotTouched.h"

#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/behaviorExternalInterface.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/beiRobotInfo.h"
#include "engine/components/sensors/touchSensorComponent.h"
#include "engine/cozmoContext.h"
#include "engine/robot.h"

#include "coretech/common/engine/jsonTools.h"
#include "coretech/common/engine/utils/timer.h"

#include <limits>

namespace Anki {
namespace Vector {

namespace{
  const char* kMinTouchTimeKey = "minTouchTime";
  const char* kWaitForReleaseKey = "waitForRelease";
  const char* kWaitForReleaseTimeoutKey = "waitForReleaseTimeout_s";
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ConditionRobotTouched::ConditionRobotTouched(const Json::Value& config)
: IBEICondition(config)
, _kMinTouchTime(0.0f)
{
  _kMinTouchTime = JsonTools::ParseFloat(config,
      kMinTouchTimeKey,
      "ConditionRobotTouched.ConfigError.NeedsMinTouchTime");

  _waitForRelease = config.get(kWaitForReleaseKey, false).asBool();
  if( _waitForRelease ) {
    _waitForReleaseTimeout_s = JsonTools::ParseFloat(config,
                                                     kWaitForReleaseTimeoutKey,
                                                     "ConditionRobotTouched.ConfigError.NeedsWaitTimeout");
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool ConditionRobotTouched::AreConditionsMetInternal(BehaviorExternalInterface& behaviorExternalInterface) const
{
  const bool touchGestureRcvd = IsReceivingTouch(behaviorExternalInterface);

  if( !_waitForRelease ) {
    // simple case, true as long as touch is happening
    return touchGestureRcvd;
  }
  else {
    const auto now = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
    if( touchGestureRcvd ) {
      _timePressed_s = now;
      // wait for release means this must be false while pressed
      return false;
    }
    else if( _timePressed_s < 0.0f ) {
      // never pressed
      return false;
    }
    else {
      const bool recentRelease = ( now - _timePressed_s ) <= _waitForReleaseTimeout_s;
      // return true if we're within the recent release timeout, false otherwise
      return recentRelease;
    }
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ConditionRobotTouched::SetActiveInternal(BehaviorExternalInterface& bei, bool isActive)
{
  // reset touch time if we're using it so it doesn't trigger right when we activate
  if( isActive && _waitForRelease ) {
    _timePressed_s = -1.0f;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool ConditionRobotTouched::IsReceivingTouch(BehaviorExternalInterface& behaviorExternalInterface) const
{
  const auto now = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
  bool ret = false;
  const bool currPressed = behaviorExternalInterface.GetTouchSensorComponent().GetIsPressed();
  const auto touchPressTime = behaviorExternalInterface.GetTouchSensorComponent().GetTouchPressTime();
  if (currPressed && (now-touchPressTime)>_kMinTouchTime) {
    ret = true;
  }
  return ret;
}


}
}


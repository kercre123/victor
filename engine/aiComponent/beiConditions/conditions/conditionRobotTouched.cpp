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
namespace Cozmo {

namespace{
  const char* kMinTouchTimeKey = "minTouchTime";
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ConditionRobotTouched::ConditionRobotTouched(const Json::Value& config)
: IBEICondition(config)
, _kMinTouchTime(0.0f)
{
  _kMinTouchTime = JsonTools::ParseFloat(config,
      kMinTouchTimeKey,
      "ConditionRobotTouched.ConfigError.NeedsMinTouchTime");
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool ConditionRobotTouched::AreConditionsMetInternal(BehaviorExternalInterface& behaviorExternalInterface) const
{
  const bool touchGestureRcvd = IsReceivingTouch(behaviorExternalInterface);
  return touchGestureRcvd;
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


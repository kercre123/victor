/**
* File: strategyRobotTouchGesture.cpp
*
* Author: Arjun Menon
* Created: 10/18/2017
*
* Description: 
* Strategy that wants to run when particular TouchGesture is being detected
*
* Copyright: Anki, Inc. 2017
*
**/

#include "engine/aiComponent/stateConceptStrategies/strategyRobotTouchGesture.h"

#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/behaviorExternalInterface.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/beiRobotInfo.h"
#include "engine/components/sensors/touchSensorComponent.h"
#include "engine/cozmoContext.h"
#include "engine/robot.h"

#include "anki/common/basestation/jsonTools.h"

namespace Anki {
namespace Cozmo {

namespace{
const char* kTouchGestureKey = "touchGesture";
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
StrategyRobotTouchGesture::StrategyRobotTouchGesture(const Json::Value& config)
: IStateConceptStrategy(config)
{
  {
    const auto& touchGestureStr = JsonTools::ParseString(config,
                                                        kTouchGestureKey,
                                                        "StrategyRobotTouchGesture.ConfigError.NeedLevel");
    _targetTouchGesture = TouchGestureFromString(touchGestureStr);
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool StrategyRobotTouchGesture::AreStateConditionsMetInternal(BehaviorExternalInterface& behaviorExternalInterface) const
{
  const bool touchGestureRcvd = IsReceivingTouchGesture(behaviorExternalInterface);
  return touchGestureRcvd;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool StrategyRobotTouchGesture::IsReceivingTouchGesture(BehaviorExternalInterface& behaviorExternalInterface) const
{
  TouchGesture touchGesture = behaviorExternalInterface.GetTouchSensorComponent().GetLatestTouchGesture();
  const bool gotTargetTouchGesture = touchGesture == _targetTouchGesture;
  return gotTargetTouchGesture;

  return false;
}


}
}


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

#include "engine/behaviorSystem/wantsToRunStrategies/strategyRobotTouchGesture.h"

#include "engine/components/touchSensorComponent.h"
#include "engine/cozmoContext.h"
#include "engine/robot.h"

#include "anki/common/basestation/jsonTools.h"

namespace Anki {
namespace Cozmo {

namespace{
const char* kTouchGestureKey = "touchGesture";
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
StrategyRobotTouchGesture::StrategyRobotTouchGesture(Robot& robot, const Json::Value& config)
: IWantsToRunStrategy(robot, config)
{
  {
    const auto& touchGestureStr = JsonTools::ParseString(config,
                                                        kTouchGestureKey,
                                                        "StrategyRobotTouchGesture.ConfigError.NeedLevel");
    _targetTouchGesture = TouchGestureFromString(touchGestureStr);
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool StrategyRobotTouchGesture::WantsToRunInternal(const Robot& robot) const
{
  const bool touchGestureRcvd = IsReceivingTouchGesture(robot);
  return touchGestureRcvd;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool StrategyRobotTouchGesture::IsReceivingTouchGesture(const Robot& robot) const
{
  TouchGesture touchGesture = robot.GetTouchSensorComponent().GetLatestTouchGesture();
  const bool gotTargetTouchGesture = touchGesture == _targetTouchGesture;
  return gotTargetTouchGesture;
}


}
}


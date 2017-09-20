/**
* File: strategyRobotShaken.cpp
*
* Author: Matt Michini - Kevin M. Karol
* Created: 2017/01/11  - 7/5/17
*
* Description: Strategy for responding to robot being shaken
*
* Copyright: Anki, Inc. 2017
*
*
**/

#include "engine/aiComponent/behaviorSystem/wantsToRunStrategies/strategyRobotShaken.h"

#include "engine/aiComponent/behaviorSystem/behaviors/iBehavior.h"
#include "engine/cozmoContext.h"
#include "engine/robot.h"

namespace Anki {
namespace Cozmo {
  
namespace{
const float kAccelMagnitudeShakingStartedThreshold = 16000.f;
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
StrategyRobotShaken::StrategyRobotShaken(BehaviorExternalInterface& behaviorExternalInterface, const Json::Value& config)
: IWantsToRunStrategy(behaviorExternalInterface, config)
{
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool StrategyRobotShaken::WantsToRunInternal(BehaviorExternalInterface& behaviorExternalInterface) const
{
  // DEPRECATED - Grabbing robot to support current cozmo code, but this should
  // be removed
  const Robot& robot = behaviorExternalInterface.GetRobot();
  // trigger this behavior when the filtered total accelerometer magnitude data exceeds a threshold
  bool shouldTrigger = (robot.GetHeadAccelMagnitudeFiltered() > kAccelMagnitudeShakingStartedThreshold);
  
  // add a check for offTreadsState?
  return shouldTrigger;
}

} // namespace Cozmo
} // namespace Anki

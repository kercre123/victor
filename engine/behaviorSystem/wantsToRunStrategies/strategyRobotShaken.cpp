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

#include "engine/behaviorSystem/wantsToRunStrategies/strategyRobotShaken.h"

#include "engine/behaviorSystem/behaviors/iBehavior.h"
#include "engine/cozmoContext.h"
#include "engine/robot.h"

namespace Anki {
namespace Cozmo {
  
namespace{
const float kAccelMagnitudeShakingStartedThreshold = 16000.f;
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
StrategyRobotShaken::StrategyRobotShaken(Robot& robot, const Json::Value& config)
: IWantsToRunStrategy(robot, config)
{
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool StrategyRobotShaken::WantsToRunInternal(const Robot& robot) const
{
  // trigger this behavior when the filtered total accelerometer magnitude data exceeds a threshold
  bool shouldTrigger = (robot.GetHeadAccelMagnitudeFiltered() > kAccelMagnitudeShakingStartedThreshold);
  
  // add a check for offTreadsState?
  return shouldTrigger;
}

} // namespace Cozmo
} // namespace Anki

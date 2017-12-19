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

#include "engine/aiComponent/stateConceptStrategies/strategyRobotShaken.h"

#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/beiRobotInfo.h"
#include "engine/aiComponent/behaviorComponent/behaviors/iCozmoBehavior.h"
#include "engine/cozmoContext.h"
#include "engine/robot.h"

namespace Anki {
namespace Cozmo {
  
namespace{
const float kAccelMagnitudeShakingStartedThreshold = 16000.f;
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
StrategyRobotShaken::StrategyRobotShaken(const Json::Value& config)
: IStateConceptStrategy(config)
{
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool StrategyRobotShaken::AreStateConditionsMetInternal(BehaviorExternalInterface& behaviorExternalInterface) const
{
  const auto& robotInfo = behaviorExternalInterface.GetRobotInfo();
  // trigger this behavior when the filtered total accelerometer magnitude data exceeds a threshold
  bool shouldTrigger = (robotInfo.GetHeadAccelMagnitudeFiltered() > kAccelMagnitudeShakingStartedThreshold);
  
  // add a check for offTreadsState?
  return shouldTrigger;
}

} // namespace Cozmo
} // namespace Anki

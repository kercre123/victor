/**
* File: strategyRobotPlacedOnSlope.cpp
*
* Author: Matt Michini - Kevin M. Karol
* Created: 01/25/17    - 7/5/17
*
* Description: Strategy which wants to run when a robot has been placed down
* on an incline/slope.
*
* Copyright: Anki, Inc. 2017
*
*
**/


#include "engine/aiComponent/beiConditions/conditions/conditionRobotPlacedOnSlope.h"

#include "engine/aiComponent/behaviorComponent/behaviors/iCozmoBehavior.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/beiRobotInfo.h"
#include "engine/externalInterface/externalInterface.h"

#include "coretech/common/engine/utils/timer.h"
#include "clad/types/robotStatusAndActions.h"

namespace Anki {
namespace Cozmo {
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ConditionRobotPlacedOnSlope::ConditionRobotPlacedOnSlope(const Json::Value& config, BEIConditionFactory& factory)
  : IBEICondition(config, factory)
{
  
}
  

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool ConditionRobotPlacedOnSlope::AreConditionsMetInternal(BehaviorExternalInterface& behaviorExternalInterface) const
{
  const auto& robotInfo = behaviorExternalInterface.GetRobotInfo();
  
  // Grab the current time:
  const double now = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
  
  // Determine if the robot is moving at all by checking the gyros:
  const GyroData& g = robotInfo.GetHeadGyroData();
  const bool isMovingNow = std::max({std::abs(g.x), std::abs(g.y), std::abs(g.z)}) > _kIsMovingNowGyroThreshold_degPerSec;
  
  if (isMovingNow) {
    _lastMovingTime = now;
  }

  // Only considered isStationary if robot has been not moving for a given time:
  const bool isStationary = (now - _lastMovingTime > _kIsStationaryTimeThreshold_sec);
  
  // Check pitch angle to see if we're inclined (but not too far as to interrupt 'OnBack' behavior):
  const float pitch_deg = robotInfo.GetPitchAngle().getDegrees();
  const bool isOnIncline = (pitch_deg > _kOnInclinePitchThresholdLow_deg) && (pitch_deg < _kOnInclinePitchThresholdHigh_deg);
  
  // Keep track of the last time the robot was reporting PickedUp state:
  const bool isPickedUp = robotInfo.IsPickedUp();
  
  if (isPickedUp) {
    _lastPickedUpTime = now;
  }

  const bool recentlyTransitionedToNotPickedUp = !isPickedUp && (now - _lastPickedUpTime < _kTransitionToNotPickedUpTimeThreshold_sec);
  
  // Trigger the behavior?
  bool shouldTrigger = isOnIncline && isStationary && (isPickedUp || recentlyTransitionedToNotPickedUp);

  // Final check - only trigger this behavior if the OffTreadsState makes sense
  const OffTreadsState ot = robotInfo.GetOffTreadsState();
  if ( (ot != OffTreadsState::OnTreads) && (ot != OffTreadsState::InAir) ) {
    shouldTrigger = false;
  }
  
  return shouldTrigger;
}

  
} // namespace Cozmo
} // namespace Anki

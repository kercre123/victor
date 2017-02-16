/**
 * File: ReactionTriggerStrategyRobotPlacedOnSlope.cpp
 *
 * Author: Matt Michini
 * Created: 01/25/17
 *
 * Description: Reaction Trigger strategy for responding to
 *
 * Copyright: Anki, Inc. 2017
 *
 *
 **/


#include "anki/cozmo/basestation/behaviorSystem/reactionTriggerStrategies/reactionTriggerStrategyRobotPlacedOnSlope.h"
#include "anki/cozmo/basestation/behaviors/iBehavior.h"
#include "anki/cozmo/basestation/externalInterface/externalInterface.h"
#include "anki/cozmo/basestation/robot.h"

#include "anki/common/basestation/utils/timer.h"
#include "clad/types/robotStatusAndActions.h"

namespace Anki {
namespace Cozmo {
  
  
namespace{
static const char* kTriggerStrategyName = "Trigger Strategy Placed On Slope";
}
  
ReactionTriggerStrategyRobotPlacedOnSlope::ReactionTriggerStrategyRobotPlacedOnSlope(Robot& robot, const Json::Value& config)
: IReactionTriggerStrategy(robot, config, kTriggerStrategyName)
{
  
}

  
bool ReactionTriggerStrategyRobotPlacedOnSlope::ShouldTriggerBehavior(const Robot& robot, const IBehavior* behavior)
{
  // Grab the current time:
  const double now = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
  
  // Determine if the robot is moving at all by checking the gyros:
  const GyroData& g = robot.GetHeadGyroData();
  const bool isMovingNow = std::max({std::abs(g.x), std::abs(g.y), std::abs(g.z)}) > _kIsMovingNowGyroThreshold_degPerSec;
  
  if (isMovingNow) {
    _lastMovingTime = now;
  }

  // Only considered isStationary if robot has been not moving for a given time:
  const bool isStationary = (now - _lastMovingTime > _kIsStationaryTimeThreshold_sec);
  
  // Check pitch angle to see if we're inclined (but not too far as to interrupt 'OnBack' behavior):
  const float pitch_deg = robot.GetPitchAngle().getDegrees();
  const bool isOnIncline = (pitch_deg > _kOnInclinePitchThresholdLow_deg) && (pitch_deg < _kOnInclinePitchThresholdHigh_deg);
  
  // Keep track of the last time the robot was reporting PickedUp state:
  const bool isPickedUp = robot.IsPickedUp();
  
  if (isPickedUp) {
    _lastPickedUpTime = now;
  }

  const bool recentlyTransitionedToNotPickedUp = !isPickedUp && (now - _lastPickedUpTime < _kTransitionToNotPickedUpTimeThreshold_sec);
  
  // Trigger the behavior?
  bool shouldTrigger = isOnIncline && isStationary && (isPickedUp || recentlyTransitionedToNotPickedUp);

  // Final check - only trigger this behavior if the OffTreadsState makes sense
  const OffTreadsState ot = robot.GetOffTreadsState();
  if ( (ot != OffTreadsState::OnTreads) && (ot != OffTreadsState::InAir) ) {
    shouldTrigger = false;
  }
  
  return shouldTrigger && behavior->IsRunnable(ReactionTriggerConst::kNoPreReqs);
}

  
} // namespace Cozmo
} // namespace Anki

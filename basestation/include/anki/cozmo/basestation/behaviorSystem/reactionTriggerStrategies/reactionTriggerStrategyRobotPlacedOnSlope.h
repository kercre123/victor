/**
 * File: ReactionTriggerStrategyRobotPlacedOnSlope.h
 *
 * Author: Matt Michini
 * Created: 01/25/17
 *
 * Description: Reaction Trigger strategy for responding to robot being placed down on an incline/slope.
 *
 * Copyright: Anki, Inc. 2017
 *
 *
 **/

#ifndef __Cozmo_Basestation_BehaviorSystem_ReactionTriggerStrategyRobotPlacedOnSlope_H__
#define __Cozmo_Basestation_BehaviorSystem_ReactionTriggerStrategyRobotPlacedOnSlope_H__

#include "anki/cozmo/basestation/behaviorSystem/reactionTriggerStrategies/iReactionTriggerStrategy.h"

namespace Anki {
namespace Cozmo {

class ReactionTriggerStrategyRobotPlacedOnSlope : public IReactionTriggerStrategy{
public:
  ReactionTriggerStrategyRobotPlacedOnSlope(Robot& robot, const Json::Value& config);

  virtual bool ShouldTriggerBehavior(const Robot& robot, const IBehavior* behavior) override;
  virtual bool ShouldResumeLastBehavior() const override { return false; }
  virtual bool CanTriggerWhileTriggeredBehaviorRunning() const override { return true; }

private:
  
  // Last time the robot was moving and picked up:
  double _lastMovingTime = 0.0;
  double _lastPickedUpTime = 0.0;

  // pitch angle thresholds between which robot is considered "OnIncline"
  const float _kOnInclinePitchThresholdLow_deg = 10.f;
  const float _kOnInclinePitchThresholdHigh_deg = 55.f;

  // if any gyro reading is above this threshold, robot is considered "moving"
  const float _kIsMovingNowGyroThreshold_degPerSec = 0.01f;
  
  // time for which robot must not be moving at all to be considered "stationary"
  const double _kIsStationaryTimeThreshold_sec = 0.40;
  
  // time after transition from PickedUp to not PickedUp for which robot is consider "just transitioned to not PickedUp"
  const double _kTransitionToNotPickedUpTimeThreshold_sec = 1.5;
  
};


} // namespace Cozmo
} // namespace Anki

#endif //

/**
* File: strategyRobotPlacedOnSlope.h
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

#ifndef __Cozmo_Basestation_BehaviorSystem_WantsToRunStrategies_ConditionRobotPlacedOnSlope_H__
#define __Cozmo_Basestation_BehaviorSystem_WantsToRunStrategies_ConditionRobotPlacedOnSlope_H__

#include "engine/aiComponent/beiConditions/iBEICondition.h"

namespace Anki {
namespace Vector {

class ConditionRobotPlacedOnSlope : public IBEICondition{
public:
  explicit ConditionRobotPlacedOnSlope(const Json::Value& config);

protected:
  virtual bool AreConditionsMetInternal(BehaviorExternalInterface& behaviorExternalInterface) const override;

private:
  // Last time the robot was moving and picked up:
  mutable double _lastMovingTime = 0.0;
  mutable double _lastPickedUpTime = 0.0;

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


} // namespace Vector
} // namespace Anki

#endif // __Cozmo_Basestation_BehaviorSystem_WantsToRunStrategies_ConditionRobotPlacedOnSlope_H__

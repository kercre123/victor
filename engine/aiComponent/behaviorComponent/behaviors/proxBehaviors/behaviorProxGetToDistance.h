/**
 * File: behaviorProxGetToDistance.h
 *
 * Author: Kevin M. Karol
 * Created: 12/11/17
 *
 * Description: Behavior that uses a graph based driving profile in order to drive
 * the robot to the requested prox sensor reading distance
 *
 * Copyright: Anki, Inc. 2017
 *
 **/

#ifndef __Cozmo_Basestation_Behaviors_ProxBehaviors_BehaviorProxGetToDistance_H__
#define __Cozmo_Basestation_Behaviors_ProxBehaviors_BehaviorProxGetToDistance_H__

#include "engine/aiComponent/behaviorComponent/behaviors/iCozmoBehavior.h"
#include "util/graphEvaluator/graphEvaluator2d.h"

namespace Anki {
namespace Cozmo {


class BehaviorProxGetToDistance : public ICozmoBehavior
{
private:
  // Enforce creation through BehaviorContainer
  friend class BehaviorContainer;
  BehaviorProxGetToDistance(const Json::Value& config);


public:
  virtual bool WantsToBeActivatedBehavior(BehaviorExternalInterface& behaviorExternalInterface) const override;

protected:
  virtual void GetBehaviorOperationModifiers(BehaviorOperationModifiers& modifiers) const override {
    modifiers.wantsToBeActivatedWhenCarryingObject = true;
    modifiers.behaviorAlwaysDelegates = false;
  }

  virtual void OnBehaviorActivated(BehaviorExternalInterface& behaviorExternalInterface) override;
  virtual void OnBehaviorDeactivated(BehaviorExternalInterface& behaviorExternalInterface) override;
  
  // allows the reaction to interrupt itself
  virtual void BehaviorUpdate(BehaviorExternalInterface& behaviorExternalInterface) override;
  
  bool ShouldRecalculateDrive(BehaviorExternalInterface& behaviorExternalInterface);
  bool IsWithinGoalTolerence(BehaviorExternalInterface& behaviorExternalInterface) const;

  float CalculateDistanceToDrive(BehaviorExternalInterface& behaviorExternalInterface) const;
  const Util::GraphEvaluator2d::Node* GetNodeClosestToDistance(const u16 distance, bool floor) const;
  
private:
  struct ProxDistanceParams{
    bool shouldEndWhenGoalReached = false;
    float goalDistance_mm = 0.f;
    float tolarance_mm = 0.f;
    Util::GraphEvaluator2d distMMToSpeedMMGraph;
  } _params;

  Pose3d _previousProxObjectPose;

  
}; // class BehaviorAcknowledgeCubeMoved

  
} // namespace Cozmo
} // namespace Anki

#endif // __Cozmo_Basestation_Behaviors_ProxBehaviors_BehaviorProxGetToDistance_H__

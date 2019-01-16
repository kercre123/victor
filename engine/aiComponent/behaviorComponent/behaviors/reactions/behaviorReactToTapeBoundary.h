/**
 * File: BehaviorReactToTapeBoundary.h
 *
 * Author: Matt Michini
 * Created: 2019-01-08
 *
 * Description: Reacts to a boundary made of retro-reflective tape or other surface that dazzles the cliff sensors
 *
 * Copyright: Anki, Inc. 2019
 *
 **/

#ifndef __Engine_AiComponent_BehaviorComponent_Behaviors_BehaviorReactToTapeBoundary__
#define __Engine_AiComponent_BehaviorComponent_Behaviors_BehaviorReactToTapeBoundary__
#pragma once

#include "engine/aiComponent/behaviorComponent/behaviors/iCozmoBehavior.h"

#include "engine/components/sensors/cliffSensorComponent.h"

namespace Anki {
namespace Vector {

class BehaviorReactToTapeBoundary : public ICozmoBehavior
{
public: 
  virtual ~BehaviorReactToTapeBoundary();

protected:

  // Enforce creation through BehaviorFactory
  friend class BehaviorFactory;
  explicit BehaviorReactToTapeBoundary(const Json::Value& config);  

  virtual void GetBehaviorOperationModifiers(BehaviorOperationModifiers& modifiers) const override;
  virtual void GetAllDelegates(std::set<IBehavior*>& delegates) const override;
  virtual void GetBehaviorJsonKeys(std::set<const char*>& expectedKeys) const override;
  
  virtual bool WantsToBeActivatedBehavior() const override;
  virtual void OnBehaviorActivated() override;
  virtual void OnBehaviorDeactivated() override;
  virtual void BehaviorUpdate() override;

private:

  // returns mask indicating which cliff sensors are _currently_ seeing the boundary
  uint8_t GetCliffsDetectingBoundary() const;
  
  void TransitionToInitialReaction();
  void TransitionToApproachBoundary();
  void TransitionToAlignWithBoundary();
  void TransitionToRefineBoundaryPose();
  
  void TransitionToApproachBoundaryLeft();
  void TransitionToApproachBoundaryRight();
  
  void TransitionToTraceBoundaryRight();
  void TransitionToTraceBoundaryLeft();
  void TransitionToViewBoundary();
  
  struct InstanceConfig {
    InstanceConfig() {}

    bool doLineFollowing = false;
  };

  struct DynamicVariables {
    DynamicVariables() {}

    // The initial 'rough' estimate of where we first contacted the boundary
    Pose3d initialBoundaryPose;
    
    // The poses of the cliff sensors when they first hit the boundary (during the approach phase). This is used to
    // refine the initial boundary pose estimate.
    Pose3d boundaryFL;
    Pose3d boundaryFR;
    
    Pose3d boundaryLeftEndPoint;
    Pose3d boundaryRightEndPoint;
    
    bool recordBoundaryCliffPoses = false;
    bool lineFollowingLeft = false;
    bool lineFollowingRight = false;
  };

  InstanceConfig _iConfig;
  DynamicVariables _dVars;
  
};

} // namespace Vector
} // namespace Anki

#endif // __Engine_AiComponent_BehaviorComponent_Behaviors_BehaviorReactToTapeBoundary__

/**
 * File: BehaviorPutDownBlockAtPose.h
 *
 * Author: Sam Russell
 * Created: 2018-10-16
 *
 * Description: If holding a cube, take it to the defined pose (current pose if none is set) and set it down using the 
 *              prescribed animation trigger (AnimationTrigger::PutDownBlockPutDown if none is set)
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#ifndef __Engine_AiComponent_BehaviorComponent_Behaviors_BehaviorPutDownBlockAtPose__
#define __Engine_AiComponent_BehaviorComponent_Behaviors_BehaviorPutDownBlockAtPose__
#pragma once

#include "engine/aiComponent/behaviorComponent/behaviors/iCozmoBehavior.h"

namespace Anki {
namespace Vector {

// forward declerations
class CarryingComponent;

class BehaviorPutDownBlockAtPose : public ICozmoBehavior
{
public: 
  virtual ~BehaviorPutDownBlockAtPose();

  // These settings are reset to defaults in OnBehaviorDeactivated
  void SetDestinationPose(const Pose3d& pose); 
  void SetPutDownAnimTrigger(const AnimationTrigger& animTrigger);

protected:

  // Enforce creation through BehaviorFactory
  friend class BehaviorFactory;
  explicit BehaviorPutDownBlockAtPose(const Json::Value& config);

  virtual void GetBehaviorOperationModifiers(BehaviorOperationModifiers& modifiers) const override;
  virtual void GetAllDelegates(std::set<IBehavior*>& delegates) const override {}
  virtual void GetBehaviorJsonKeys(std::set<const char*>& expectedKeys) const override;
  
  virtual bool WantsToBeActivatedBehavior() const override;
  virtual void OnBehaviorActivated() override;
  virtual void OnBehaviorDeactivated() override;
  virtual void BehaviorUpdate() override;

private:

  enum class PutDownCubeState{
    DriveToDestinationPose,
    PutDownBlock,
    LookDownAtBlock
  };

  void TransitionToDriveToDestinationPose();
  void TransitionToPutDownBlock();
  void TransitionToLookDownAtBlock();
  IActionRunner* CreateLookAfterPlaceAction();

  struct InstanceConfig {
    InstanceConfig();
  };

  struct DynamicVariables {
    DynamicVariables();
    PutDownCubeState state;
    struct Persistent{
      Persistent();
      Pose3d           destinationPose;
      AnimationTrigger putDownAnimTrigger;
      bool             customDestPending;
    } persistent;
  };

  InstanceConfig _iConfig;
  DynamicVariables _dVars;
  
};

} // namespace Vector
} // namespace Anki

#endif // __Engine_AiComponent_BehaviorComponent_Behaviors_BehaviorPutDownBlockAtPose__

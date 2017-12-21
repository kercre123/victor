/**
 * File: behaviorCantHandleTallStack.h
 *
 * Author: Kevin M. Karol
 * Created: 2016-08-29
 *
 * Description: Behavior to unstack a large stack of cubes before knock over cubes is unlocked
 *
 * Copyright: Anki, Inc. 2016
 *
 **/

#ifndef __Cozmo_Basestation_Behaviors_BehaviorCantHandleTallStack_H__
#define __Cozmo_Basestation_Behaviors_BehaviorCantHandleTallStack_H__

#include "anki/common/basestation/objectIDs.h"
#include "engine/aiComponent/behaviorComponent/behaviors/iCozmoBehavior.h"
#include "engine/blockWorld/blockConfiguration.h"
#include "engine/blockWorld/stackConfigurationContainer.h"
#include "clad/types/animationTrigger.h"

namespace Anki {
namespace Cozmo {

class BehaviorCantHandleTallStack : public ICozmoBehavior
{
protected:
  // Enforce creation through BehaviorContainer
  friend class BehaviorContainer;
  BehaviorCantHandleTallStack(const Json::Value& config);

  virtual void OnBehaviorActivated(BehaviorExternalInterface& behaviorExternalInterface) override;
  virtual void OnBehaviorDeactivated(BehaviorExternalInterface& behaviorExternalInterface) override;
  
  virtual bool WantsToBeActivatedBehavior(BehaviorExternalInterface& behaviorExternalInterface) const override;
  virtual bool CarryingObjectHandledInternally() const override {return false;}
  
  virtual void AlwaysHandleInScope(const EngineToGameEvent& event, BehaviorExternalInterface& behaviorExternalInterface) override;
  
private:
  mutable BlockConfigurations::StackWeakPtr _currentTallestStack;
  
  bool _isLastReactionPoseValid;
  Pose3d _lastReactionBasePose;
  uint8_t _minStackHeight;
  
  // loaded in from json
  float _lookingInitialWait_s;
  float _lookingDownWait_s;
  float _lookingTopWait_s;
  float _minBlockMovedThreshold_mm;
  
  enum class DebugState {
    LookingUpAndDown,
    Disapointment
  };
  
  void TransitionToLookingUpAndDown(BehaviorExternalInterface& behaviorExternalInterface);
  void TransitionToDisapointment(BehaviorExternalInterface& behaviorExternalInterface);

  
  virtual void ClearStack();
  virtual void UpdateTargetStack(const BehaviorExternalInterface& behaviorExternalInterface) const;

};


}
}


#endif

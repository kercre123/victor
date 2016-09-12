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
#include "anki/cozmo/basestation/behaviors/behaviorInterface.h"
#include "clad/types/animationTrigger.h"

namespace Anki {
namespace Cozmo {

class Robot;

class BehaviorCantHandleTallStack : public IReactionaryBehavior
{
protected:
  // Enforce creation through BehaviorFactory
  friend class BehaviorFactory;
  BehaviorCantHandleTallStack(Robot& robot, const Json::Value& config);

  virtual Result InitInternalReactionary(Robot& robot) override;
  virtual void   StopInternalReactionary(Robot& robot) override;
  virtual bool ShouldComputationallySwitch(const Robot& robot) override;
  
  virtual bool IsRunnableInternalReactionary(const Robot& robot) const override;
  virtual bool CarryingObjectHandledInternally() const override {return false;}
  virtual bool ShouldResumeLastBehavior() const override { return false;}
  
  virtual void AlwaysHandleInternal(const EngineToGameEvent& event, const Robot& robot) override;
  
private:
  
  ObjectID _baseBlockID;
  uint8_t _stackHeight;
  bool _objectObservedChanged;
  
  bool _baseBlockPoseValid;
  Pose3d _lastReactionBasePose;
  
  
  // For checking computational switch
  ObjectID _lastObservedObject;
  
  // loaded in from json
  float _lookingInitialWait_s;
  float _lookingDownWait_s;
  float _lookingTopWait_s;
  float _minBlockMovedThreshold_mm_sqr;
  
  enum class DebugState {
    ReactingToStack,
    LookingAtStack,
    LookingUpAndDown,
    Disapointment
  };
  
  void TransitionToReactingToStack(Robot& robot);
  void TransitionToLookingAtStack(Robot& robot);
  void TransitionToLookingUpAndDown(Robot& robot);
  void TransitionToDisapointment(Robot& robot);

  
  virtual void ResetBehavior();
  virtual void UpdateTargetStack(const Robot& robot);

};


}
}


#endif

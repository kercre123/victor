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
#include "anki/cozmo/basestation/blockWorld/blockConfiguration.h"
#include "clad/types/animationTrigger.h"

namespace Anki {
namespace Cozmo {

class Robot;

class BehaviorCantHandleTallStack : public IBehavior
{
protected:
  // Enforce creation through BehaviorFactory
  friend class BehaviorFactory;
  BehaviorCantHandleTallStack(Robot& robot, const Json::Value& config);

  virtual Result InitInternal(Robot& robot) override;
  virtual void   StopInternal(Robot& robot) override;
  
  virtual bool IsRunnableInternal(const Robot& robot) const override;
  virtual bool CarryingObjectHandledInternally() const override {return false;}
  
  virtual void AlwaysHandle(const EngineToGameEvent& event, const Robot& robot) override;
  
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
  
  void TransitionToLookingUpAndDown(Robot& robot);
  void TransitionToDisapointment(Robot& robot);

  
  virtual void ClearStack();
  virtual void UpdateTargetStack(const Robot& robot) const;

};


}
}


#endif

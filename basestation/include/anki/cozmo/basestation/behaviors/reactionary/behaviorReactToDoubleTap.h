/**
 * File: behaviorReactToDoubleTap.h
 *
 * Author: Al Chaussee
 * Created: 11/9/2016
 *
 * Description: Behavior to react when a unknown or dirty cube is double tapped
 *
 * Copyright: Anki, Inc. 2016
 *
 **/

#ifndef __Cozmo_Basestation_Behaviors_BehaviorReactToDoubleTap_H__
#define __Cozmo_Basestation_Behaviors_BehaviorReactToDoubleTap_H__

#include "anki/cozmo/basestation/behaviors/behaviorInterface.h"
#include <vector>

namespace Anki {
namespace Cozmo {

class BehaviorReactToDoubleTap : public IReactionaryBehavior
{
private:
  // Enforce creation through BehaviorFactory
  friend class BehaviorFactory;
  BehaviorReactToDoubleTap(Robot& robot, const Json::Value& config);
  
public:
  virtual bool IsRunnableInternalReactionary(const Robot& robot) const override;
  virtual bool ShouldResumeLastBehavior() const override { return true; }
  virtual bool ShouldComputationallySwitch(const Robot& robot) override { return true; }
  virtual bool ShouldInterruptOtherReactionaryBehavior() override { return false; }
  
protected:
  virtual Result InitInternalReactionary(Robot& robot) override;
  virtual void StopInternalReactionary(Robot& robot) override;
  virtual void StopInternalFromDoubleTap(Robot& robot) override { StopInternalReactionary(robot); }
  
  // Returns true if the double tapped object is valid to be reacted to
  bool IsTappedObjectValid(const Robot& robot) const;
  
  void TransitionToDriveToCube(Robot& robot);
  
  void TransitionToSearchForCube(Robot& robot);
  
private:
  ObjectID _lastObjectReactedTo;
  
  bool _objectInCurrentFrame = true;
  
  // Whether or not we are turning towards a ghost object
  bool _turningTowardsGhostObject = false;
  
  // Whether or not we should leaveObjectTapInteraction when this behavior stops
  bool _leaveTapInteractionOnStop = false;
};

}
}

#endif

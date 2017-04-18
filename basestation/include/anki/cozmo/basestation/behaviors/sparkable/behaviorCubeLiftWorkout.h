/**
 * File: behaviorCubeLiftWorkout.h
 *
 * Author: Brad Neuman
 * Created: 2016-10-24
 *
 * Description: Behavior to lift a cube and do a "workout" routine with it, with animations and parameters
 *              defined in json
 *
 * Copyright: Anki, Inc. 2016
 *
 **/

#ifndef __Cozmo_Basestation_Behaviors_Sparkable_BehaviorCubeLiftWorkout_H__
#define __Cozmo_Basestation_Behaviors_Sparkable_BehaviorCubeLiftWorkout_H__

#include "anki/cozmo/basestation/behaviors/iBehavior.h"

#include "anki/common/basestation/objectIDs.h"
#include "clad/types/animationTrigger.h"

namespace Anki {
namespace Cozmo {

class BehaviorCubeLiftWorkout : public IBehavior
{
protected:
  using super = IBehavior;
  
  // Enforce creation through BehaviorFactory
  friend class BehaviorFactory;
  BehaviorCubeLiftWorkout(Robot& robot, const Json::Value& config);

public:

  virtual bool CarryingObjectHandledInternally() const override { return true; }

protected:

  virtual bool IsRunnableInternal(const BehaviorPreReqRobot& preReqData) const override;

  virtual Result InitInternal(Robot& robot) override;

  // behavior can't be resumed (may be selected again, which should fast-forward if needed)
  virtual Result ResumeInternal(Robot& robot) override { return RESULT_FAIL; }

  virtual Status UpdateInternal(Robot& robot) override;
  
  virtual void   StopInternal(Robot& robot) override;

  void TransitionToPickingUpCube(Robot& robot);
  void TransitionToPostLiftAnim(Robot& robot);
  void TransitionToStrongLifts(Robot& robot);
  void TransitionToWeakPose(Robot& robot);
  void TransitionToWeakLifts(Robot& robot);
  void TransitionToPuttingDown(Robot& robot);
  void TransitionToCheckPutDown(Robot& robot);
  void TransitionToManualPutDown(Robot& robot);
  void EndIteration(Robot& robot);

  // set by init to the number of lifts we should do, then decremented in the state machine
  unsigned int _numStrongLiftsToDo = 0;
  unsigned int _numWeakLiftsToDo = 0;

  bool _shouldBeCarrying = false;

  ObjectID _targetBlockID;
};

}
}
 

#endif

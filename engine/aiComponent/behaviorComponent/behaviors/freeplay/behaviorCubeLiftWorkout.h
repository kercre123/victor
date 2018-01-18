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

#include "engine/aiComponent/behaviorComponent/behaviors/iCozmoBehavior.h"

#include "coretech/common/engine/objectIDs.h"
#include "clad/types/animationTrigger.h"

namespace Anki {
namespace Cozmo {

class BehaviorCubeLiftWorkout : public ICozmoBehavior
{
protected:
  using super = ICozmoBehavior;
  
  // Enforce creation through BehaviorContainer
  friend class BehaviorContainer;
  BehaviorCubeLiftWorkout(const Json::Value& config);

  virtual void GetBehaviorOperationModifiers(BehaviorOperationModifiers& modifiers) const override {
    modifiers.wantsToBeActivatedWhenCarryingObject = true;
  }


  virtual bool WantsToBeActivatedBehavior() const override;

  virtual void OnBehaviorActivated() override;
  virtual void BehaviorUpdate() override;

  virtual void OnBehaviorDeactivated() override;

  void TransitionToPickingUpCube();
  void TransitionToPostLiftAnim();
  void TransitionToStrongLifts();
  void TransitionToWeakPose();
  void TransitionToWeakLifts();
  void TransitionToPuttingDown();
  void TransitionToCheckPutDown();
  void TransitionToManualPutDown();
  void EndIteration();

  // set by init to the number of lifts we should do, then decremented in the state machine
  unsigned int _numStrongLiftsToDo = 0;
  unsigned int _numWeakLiftsToDo = 0;

  bool _shouldBeCarrying = false;

  ObjectID _targetBlockID;
};

}
}
 

#endif

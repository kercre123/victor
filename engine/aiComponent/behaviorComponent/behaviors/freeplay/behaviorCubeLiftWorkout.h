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

public:

  virtual bool CarryingObjectHandledInternally() const override { return true; }

protected:

  virtual bool WantsToBeActivatedBehavior(BehaviorExternalInterface& behaviorExternalInterface) const override;

  virtual void OnBehaviorActivated(BehaviorExternalInterface& behaviorExternalInterface) override;
  virtual void BehaviorUpdate(BehaviorExternalInterface& behaviorExternalInterface) override;

  virtual void OnBehaviorDeactivated(BehaviorExternalInterface& behaviorExternalInterface) override;

  void TransitionToPickingUpCube(BehaviorExternalInterface& behaviorExternalInterface);
  void TransitionToPostLiftAnim(BehaviorExternalInterface& behaviorExternalInterface);
  void TransitionToStrongLifts(BehaviorExternalInterface& behaviorExternalInterface);
  void TransitionToWeakPose(BehaviorExternalInterface& behaviorExternalInterface);
  void TransitionToWeakLifts(BehaviorExternalInterface& behaviorExternalInterface);
  void TransitionToPuttingDown(BehaviorExternalInterface& behaviorExternalInterface);
  void TransitionToCheckPutDown(BehaviorExternalInterface& behaviorExternalInterface);
  void TransitionToManualPutDown(BehaviorExternalInterface& behaviorExternalInterface);
  void EndIteration(BehaviorExternalInterface& behaviorExternalInterface);

  // set by init to the number of lifts we should do, then decremented in the state machine
  unsigned int _numStrongLiftsToDo = 0;
  unsigned int _numWeakLiftsToDo = 0;

  bool _shouldBeCarrying = false;

  ObjectID _targetBlockID;
};

}
}
 

#endif

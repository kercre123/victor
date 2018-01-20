/**
 * File: behaviorPickUpAndPutDownCube.h
 *
 * Author: Brad Neuman
 * Created: 2017-03-06
 *
 * Description: Behavior for the "pickup" spark, which picks up and then puts down a cube
 *
 * Copyright: Anki, Inc. 2017
 *
 **/

#ifndef __Cozmo_Basestation_Behaviors_Sparkable_BehaviorPickUpAndPutDownCube_H__
#define __Cozmo_Basestation_Behaviors_Sparkable_BehaviorPickUpAndPutDownCube_H__

#include "coretech/common/engine/objectIDs.h"
#include "engine/aiComponent/behaviorComponent/behaviors/iCozmoBehavior.h"

#include <vector>

namespace Anki {
namespace Cozmo {

class BehaviorPickUpAndPutDownCube : public ICozmoBehavior
{
using super = ICozmoBehavior;
protected:  
  // Enforce creation through BehaviorContainer
  friend class BehaviorContainer;
  BehaviorPickUpAndPutDownCube(const Json::Value& config);

  virtual void GetBehaviorOperationModifiers(BehaviorOperationModifiers& modifiers) const override {}
  virtual void OnBehaviorActivated() override;
  virtual bool WantsToBeActivatedBehavior() const override;
  
private:
  
  enum class State
  {
    DriveWithCube,
    PutDownCube,
  };
  
  void TransitionToDriveWithCube();
  void TransitionToPutDownCube();

  mutable ObjectID _targetBlockID;
  
}; // class BehaviorPickUpCube

} // namespace Cozmo
} // namespace Anki

#endif

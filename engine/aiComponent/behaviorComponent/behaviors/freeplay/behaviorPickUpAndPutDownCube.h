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

#include "anki/common/basestation/objectIDs.h"
#include "engine/aiComponent/behaviorComponent/behaviors/iBehavior.h"

#include <vector>

namespace Anki {
namespace Cozmo {

class BehaviorPickUpAndPutDownCube : public IBehavior
{
using super = IBehavior;
protected:  
  // Enforce creation through BehaviorContainer
  friend class BehaviorContainer;
  BehaviorPickUpAndPutDownCube(const Json::Value& config);

  virtual Result OnBehaviorActivated(BehaviorExternalInterface& behaviorExternalInterface) override;

  virtual bool WantsToBeActivatedBehavior(BehaviorExternalInterface& behaviorExternalInterface) const override;
  virtual bool CarryingObjectHandledInternally() const override { return false;}
  
private:
  
  enum class State
  {
    DriveWithCube,
    PutDownCube,
  };
  
  void TransitionToDriveWithCube(BehaviorExternalInterface& behaviorExternalInterface);
  void TransitionToPutDownCube(BehaviorExternalInterface& behaviorExternalInterface);

  mutable ObjectID _targetBlockID;
  
}; // class BehaviorPickUpCube

} // namespace Cozmo
} // namespace Anki

#endif

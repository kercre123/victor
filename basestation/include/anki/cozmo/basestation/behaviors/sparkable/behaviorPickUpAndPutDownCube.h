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
#include "anki/cozmo/basestation/behaviors/iBehavior.h"

#include <vector>

namespace Anki {
namespace Cozmo {

class BehaviorPickUpAndPutDownCube : public IBehavior
{
using super = IBehavior;
protected:  
  // Enforce creation through BehaviorFactory
  friend class BehaviorFactory;
  BehaviorPickUpAndPutDownCube(Robot& robot, const Json::Value& config);

  virtual Result InitInternal(Robot& robot) override;

  virtual bool IsRunnableInternal(const BehaviorPreReqRobot& preReqData) const override;
  virtual bool CarryingObjectHandledInternally() const override { return false;}
  
private:
  
  enum class State
  {
    DriveWithCube,
    PutDownCube,
  };
  
  void TransitionToDriveWithCube(Robot& robot);
  void TransitionToPutDownCube(Robot& robot);

  mutable ObjectID _targetBlockID;
  
}; // class BehaviorPickUpCube

} // namespace Cozmo
} // namespace Anki

#endif

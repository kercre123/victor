/**
 * File: behaviorPutDownBlock.h
 *
 * Author: Brad Neuman
 * Created: 2016-05-23
 *
 * Description: Simple behavior which puts down a block (using an animation group)
 *
 * Copyright: Anki, Inc. 2016
 *
 **/

#ifndef __Cozmo_Basestation_Behaviors_BehaviorPutDownBlock_H__
#define __Cozmo_Basestation_Behaviors_BehaviorPutDownBlock_H__

#include "anki/cozmo/basestation/behaviorSystem/behaviors/iBehavior.h"

namespace Anki {
namespace Cozmo {

class BehaviorPutDownBlock : public IBehavior
{
public:

  // TODO: Use a PlaceObjectOnGround action (with animatino) and use its VisuallyVerify
  // helper for creating an action to make sure that the "put down" animation is working. It looks down at the
  // block to make sure we have a chance to see it
  static IActionRunner* CreateLookAfterPlaceAction(Robot& robot, bool doLookAtFaceAfter=true);

protected:
  // Enforce creation through BehaviorFactory
  friend class BehaviorFactory;
  BehaviorPutDownBlock(Robot& robot, const Json::Value& config);

  virtual bool IsRunnableInternal(const BehaviorPreReqRobot& preReqData) const override;
  virtual bool CarryingObjectHandledInternally() const override {return true;}

  virtual Result InitInternal(Robot& robot) override;
  
private:

  void LookDownAtBlock(Robot& robot);

};

} // namespace Cozmo
} // namespace Anki

#endif // __Cozmo_Basestation_Behaviors_BehaviorPutDownBlock_H__

/**
* File: behaviorFeedingHungerLoop.h
*
* Author: Kevin M. Karol
* Created: 2017-3-28
*
* Description: Behavior for cozmo to anticipate energy being loaded
* into a cube as the user prepares it
*
* Copyright: Anki, Inc. 2017
*
**/

#ifndef __Cozmo_Basestation_Behaviors_Feeding_BehaviorFeedingHungerLoop_H__
#define __Cozmo_Basestation_Behaviors_Feeding_BehaviorFeedingHungerLoop_H__

#include "anki/cozmo/basestation/behaviorSystem/behaviors/iBehavior.h"

namespace Anki {
namespace Cozmo {

class BehaviorFeedingHungerLoop : public IBehavior
{
protected:
  
  // Enforce creation through BehaviorFactory
  friend class BehaviorFactory;
  BehaviorFeedingHungerLoop(Robot& robot, const Json::Value& config);
  
public:
  
  virtual bool IsRunnableInternal(const BehaviorPreReqNone& preReqData) const override;
  virtual bool CarryingObjectHandledInternally() const override {return false;}
  
  bool HasUsableFace(Robot& robot);
  
protected:
  virtual Result InitInternal(Robot& robot) override;
  
private:
  void TransitionAskForFood(Robot& robot);
  void TransitionWaitForFood(Robot& robot);
  
  
};

} // namespace Cozmo
} // namespace Anki


#endif // __Cozmo_Basestation_Behaviors_Feeding_BehaviorFeedingHungerLoop_H__


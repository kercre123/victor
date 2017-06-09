/**
* File: behaviorFeedingSearchForCube.h
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

#ifndef __Cozmo_Basestation_Behaviors_Feeding_BehaviorFeedingSearchForCube_H__
#define __Cozmo_Basestation_Behaviors_Feeding_BehaviorFeedingSearchForCube_H__

#include "anki/cozmo/basestation/behaviorSystem/behaviors/iBehavior.h"

namespace Anki {
namespace Cozmo {

class BehaviorFeedingSearchForCube : public IBehavior
{
protected:
  
  // Enforce creation through BehaviorFactory
  friend class BehaviorFactory;
  BehaviorFeedingSearchForCube(Robot& robot, const Json::Value& config);
  
public:
  
  virtual bool IsRunnableInternal(const BehaviorPreReqNone& preReqData) const override;
  virtual bool CarryingObjectHandledInternally() const override {return false;}
    
protected:
  virtual Result InitInternal(Robot& robot) override;
  
private:
  void TransitionAskForFood(Robot& robot);
  void TransitionWaitForFood(Robot& robot);
  
  
};

} // namespace Cozmo
} // namespace Anki


#endif // __Cozmo_Basestation_Behaviors_Feeding_BehaviorFeedingSearchForCube_H__


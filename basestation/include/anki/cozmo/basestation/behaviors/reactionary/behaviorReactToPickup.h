/**
 * File: behaviorReactToPickup.h
 *
 * Author: Lee
 * Created: 08/26/15
 *
 * Description: Behavior for immediately responding being picked up.
 *
 * Copyright: Anki, Inc. 2015
 *
 **/

#ifndef __Cozmo_Basestation_Behaviors_BehaviorReactToPickup_H__
#define __Cozmo_Basestation_Behaviors_BehaviorReactToPickup_H__

#include "anki/cozmo/basestation/behaviors/iBehavior.h"

namespace Anki {
namespace Cozmo {
  
class BehaviorReactToPickup : public IBehavior
{
private:
  
  // Enforce creation through BehaviorFactory
  friend class BehaviorFactory;
  BehaviorReactToPickup(Robot& robot, const Json::Value& config);
  
public:
  virtual bool IsRunnableInternal(const BehaviorPreReqNone& preReqData) const override;
  virtual bool CarryingObjectHandledInternally() const override {return true;}

protected:
  virtual Result InitInternal(Robot& robot) override;
  virtual Status UpdateInternal(Robot& robot) override;
  virtual void   StopInternal(Robot& robot) override;
  virtual bool ShouldRunWhileOffTreads() const override { return true;}

  
  void StartAnim(Robot& robot);

private:

  float _nextRepeatAnimationTime = 0.0f;
  double _repeatAnimatingMultiplier = 1;
  
}; // class BehaviorReactToPickup
  

} // namespace Cozmo
} // namespace Anki

#endif // __Cozmo_Basestation_Behaviors_BehaviorReactToPickup_H__

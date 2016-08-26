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

#include "anki/cozmo/basestation/behaviors/behaviorInterface.h"
#include "anki/common/basestation/objectIDs.h"
#include "clad/externalInterface/messageEngineToGame.h"
#include <vector>

namespace Anki {
namespace Cozmo {
  
class BehaviorReactToPickup : public IReactionaryBehavior
{
private:
  
  // Enforce creation through BehaviorFactory
  friend class BehaviorFactory;
  BehaviorReactToPickup(Robot& robot, const Json::Value& config);
  
public:

  virtual bool IsRunnableInternalReactionary(const Robot& robot) const override;
  virtual bool ShouldResumeLastBehavior() const override { return false; }
  
protected:
  
  virtual Result InitInternalReactionary(Robot& robot) override;
  virtual Status UpdateInternal(Robot& robot) override;
  virtual void   StopInternalReactionary(Robot& robot) override;
  virtual bool ShouldRunWhileOffTreads() const override { return true;}
  virtual bool ShouldComputationallySwitch(const Robot& robot) override;

  void StartAnim(Robot& robot);

private:

  double _nextRepeatAnimationTime = 0;
  double _repeatAnimatingMultiplier = 1;
  
}; // class BehaviorReactToPickup
  

} // namespace Cozmo
} // namespace Anki

#endif // __Cozmo_Basestation_Behaviors_BehaviorReactToPickup_H__

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

#include "engine/aiComponent/behaviorSystem/behaviors/iBehavior.h"

namespace Anki {
namespace Cozmo {
  
class BehaviorReactToPickup : public IBehavior
{
private:
  
  // Enforce creation through BehaviorContainer
  friend class BehaviorContainer;
  BehaviorReactToPickup(const Json::Value& config);
  
public:
  virtual bool WantsToBeActivatedBehavior(BehaviorExternalInterface& behaviorExternalInterface) const override;
  virtual bool CarryingObjectHandledInternally() const override {return true;}

protected:
  virtual Result OnBehaviorActivated(BehaviorExternalInterface& behaviorExternalInterface) override;
  virtual Status UpdateInternal_WhileRunning(BehaviorExternalInterface& behaviorExternalInterface) override;
  virtual void   OnBehaviorDeactivated(BehaviorExternalInterface& behaviorExternalInterface) override;
  virtual bool ShouldRunWhileOffTreads() const override { return true;}

  
  void StartAnim(BehaviorExternalInterface& behaviorExternalInterface);

private:

  float _nextRepeatAnimationTime = 0.0f;
  double _repeatAnimatingMultiplier = 1;
  
}; // class BehaviorReactToPickup
  

} // namespace Cozmo
} // namespace Anki

#endif // __Cozmo_Basestation_Behaviors_BehaviorReactToPickup_H__

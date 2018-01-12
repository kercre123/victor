/**
 * File: BehaviorEarnedSparks
 *
 * Author: Paul Terry
 * Created: 07/18/2017
 *
 * Description: Simple behavior for Cozmo playing an animaton when he has earned sparks
 *
 * Copyright: Anki, Inc. 2017
 *
 **/

#ifndef __Cozmo_Basestation_Behaviors_BehaviorEarnedSparks_H__
#define __Cozmo_Basestation_Behaviors_BehaviorEarnedSparks_H__

#include "engine/aiComponent/behaviorComponent/behaviors/iCozmoBehavior.h"
#include "clad/types/animationTrigger.h"


namespace Anki {
namespace Cozmo {

class BehaviorEarnedSparks : public ICozmoBehavior
{
protected:
  // Enforce creation through BehaviorContainer
  friend class BehaviorContainer;
  BehaviorEarnedSparks(const Json::Value& config);

public:
  virtual ~BehaviorEarnedSparks();

  virtual bool WantsToBeActivatedBehavior(BehaviorExternalInterface& behaviorExternalInterface) const override;

protected:
  virtual void GetBehaviorOperationModifiers(BehaviorOperationModifiers& modifiers) const override {
    modifiers.wantsToBeActivatedWhenCarryingObject = true;
  }

  virtual void OnBehaviorActivated(BehaviorExternalInterface& behaviorExternalInterface) override;
  virtual void OnBehaviorDeactivated(BehaviorExternalInterface& behaviorExternalInterface) override;

};


} // namespace Cozmo
} // namespace Anki

#endif // __Cozmo_Basestation_Behaviors_BehaviorEarnedSparks_H__

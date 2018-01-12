/**
 * File: behaviorReactToSparked.h
 *
 * Author:  Kevin M. Karol
 * Created: 2016-09-13
 *
 * Description:  Reaction that cancels other reactions when cozmo is sparked
 *
 * Copyright: Anki, Inc. 2016
 *
 **/

#ifndef __Cozmo_Basestation_Behaviors_BehaviorReactToSparked_H__
#define __Cozmo_Basestation_Behaviors_BehaviorReactToSparked_H__

#include "engine/aiComponent/behaviorComponent/behaviors/iCozmoBehavior.h"

namespace Anki {
namespace Cozmo {
  
class BehaviorReactToSparked : public ICozmoBehavior
{
protected:
  // Enforce creation through BehaviorContainer
  friend class BehaviorContainer;
  BehaviorReactToSparked(const Json::Value& config);

  virtual void GetBehaviorOperationModifiers(BehaviorOperationModifiers& modifiers) const override {
    modifiers.wantsToBeActivatedWhenOffTreads = true;
  }

  virtual void OnBehaviorActivated(BehaviorExternalInterface& behaviorExternalInterface) override;
  virtual bool WantsToBeActivatedBehavior(BehaviorExternalInterface& behaviorExternalInterface) const override;
  
}; // class BehaviorReactToSparked

  
} // namespace Cozmo
} // namespace Anki

#endif // __Cozmo_Basestation_Behaviors_BehaviorReactToSparked_H__

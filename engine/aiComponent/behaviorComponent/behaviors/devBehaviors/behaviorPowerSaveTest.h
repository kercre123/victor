/**
 * File: BehaviorPowerSaveTest.h
 *
 * Author: Brad
 * Created: 2018-06-29
 *
 * Description: Simple behavior to test power save mode (and nothing else)
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#ifndef __Engine_AiComponent_BehaviorComponent_Behaviors_BehaviorPowerSaveTest__
#define __Engine_AiComponent_BehaviorComponent_Behaviors_BehaviorPowerSaveTest__
#pragma once

#include "engine/aiComponent/behaviorComponent/behaviors/iCozmoBehavior.h"

namespace Anki {
namespace Vector {

class BehaviorPowerSaveTest : public ICozmoBehavior
{
public: 
  virtual ~BehaviorPowerSaveTest() {}

protected:

  // Enforce creation through BehaviorFactory
  friend class BehaviorFactory;
  explicit BehaviorPowerSaveTest(const Json::Value& config) : ICozmoBehavior(config) {}

  virtual void GetBehaviorOperationModifiers(BehaviorOperationModifiers& modifiers) const override {
    modifiers.wantsToBeActivatedWhenCarryingObject = true;
    modifiers.wantsToBeActivatedWhenOffTreads = true;
    modifiers.wantsToBeActivatedWhenOnCharger = true;
    modifiers.behaviorAlwaysDelegates = false;
  }
    
  virtual void GetAllDelegates(std::set<IBehavior*>& delegates) const override {}
  virtual void GetBehaviorJsonKeys(std::set<const char*>& expectedKeys) const override {}
  
  virtual bool WantsToBeActivatedBehavior() const override { return true; }
  virtual void OnBehaviorActivated() override { SmartRequestPowerSaveMode(); }
  
};

} // namespace Vector
} // namespace Anki

#endif // __Engine_AiComponent_BehaviorComponent_Behaviors_BehaviorPowerSaveTest__

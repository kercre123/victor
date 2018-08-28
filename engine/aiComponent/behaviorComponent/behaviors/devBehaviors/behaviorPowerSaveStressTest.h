/**
 * File: BehaviorPowerSaveStressTest.h
 *
 * Author: Al Chaussee
 * Created: 8/15/2018
 *
 * Description: Simple behavior to stress test power save mode (and nothing else)
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#ifndef __Engine_AiComponent_BehaviorComponent_Behaviors_BehaviorPowerSaveStressTest__
#define __Engine_AiComponent_BehaviorComponent_Behaviors_BehaviorPowerSaveStressTest__
#pragma once

#include "engine/aiComponent/behaviorComponent/behaviors/iCozmoBehavior.h"

namespace Anki {
namespace Vector {

class BehaviorPowerSaveStressTest : public ICozmoBehavior
{
public:
  virtual ~BehaviorPowerSaveStressTest() {}

protected:

  // Enforce creation through BehaviorFactory
  friend class BehaviorFactory;
  explicit BehaviorPowerSaveStressTest(const Json::Value& config) : ICozmoBehavior(config) {}

  virtual void GetBehaviorOperationModifiers(BehaviorOperationModifiers& modifiers) const override {
    modifiers.wantsToBeActivatedWhenCarryingObject = true;
    modifiers.wantsToBeActivatedWhenOffTreads = true;
    modifiers.wantsToBeActivatedWhenOnCharger = true;
    modifiers.behaviorAlwaysDelegates = false;
  }
    
  virtual void GetAllDelegates(std::set<IBehavior*>& delegates) const override {}
  virtual void GetBehaviorJsonKeys(std::set<const char*>& expectedKeys) const override {}
  
  virtual bool WantsToBeActivatedBehavior() const override { return true; }
  virtual void OnBehaviorActivated() override;

  virtual void BehaviorUpdate() override;

private:
  EngineTimeStamp_t _then = 0;
  
};

} // namespace Vector
} // namespace Anki

#endif // __Engine_AiComponent_BehaviorComponent_Behaviors_BehaviorPowerSaveStressTest__

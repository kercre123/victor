/**
 * File: behaviorOnboardingInterruptionHead.h
 *
 * Author: ross
 * Created: 2018-06-03
 *
 * Description: Keeps the head in the correct position based on onboarding stage
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#ifndef __Engine_AiComponent_BehaviorComponent_Behaviors_BehaviorOnboardingInterruptionHead__
#define __Engine_AiComponent_BehaviorComponent_Behaviors_BehaviorOnboardingInterruptionHead__
#pragma once

#include "engine/aiComponent/behaviorComponent/behaviors/iCozmoBehavior.h"

namespace Anki {
namespace Cozmo {

class BehaviorOnboardingInterruptionHead : public ICozmoBehavior
{
public: 
  virtual ~BehaviorOnboardingInterruptionHead();

protected:

  // Enforce creation through BehaviorFactory
  friend class BehaviorFactory;
  explicit BehaviorOnboardingInterruptionHead(const Json::Value& config);  

  virtual void GetBehaviorOperationModifiers(BehaviorOperationModifiers& modifiers) const override;
  virtual void GetAllDelegates(std::set<IBehavior*>& delegates) const override {}
  virtual void GetBehaviorJsonKeys(std::set<const char*>& expectedKeys) const override {}
  
  virtual bool WantsToBeActivatedBehavior() const override { return true; }
  virtual void OnBehaviorActivated() override;

private:

  struct InstanceConfig {
    InstanceConfig();
  };

  struct DynamicVariables {
    DynamicVariables();
  };

  InstanceConfig _iConfig;
  DynamicVariables _dVars;
  
};

} // namespace Cozmo
} // namespace Anki

#endif // __Engine_AiComponent_BehaviorComponent_Behaviors_BehaviorOnboardingInterruptionHead__

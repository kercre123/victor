/**
 * File: behaviorOnboardingInterruptionHead.h
 *
 * Author: ross
 * Created: 2018-06-03
 *
 * Description: Interruption to match the current animation when picked up or on charger. This
 *              has special casing around the first stage, but isn't part of that stage so that all
 *              picked up/on charger logic can be centralized
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
  
enum class AnimationTrigger : int32_t;

class BehaviorOnboardingInterruptionHead : public ICozmoBehavior
{
public: 
  virtual ~BehaviorOnboardingInterruptionHead();
  
  void SetPlayGroggyAnimations( bool isGroggy ) { _dVars.isGroggy = isGroggy; }

protected:

  // Enforce creation through BehaviorFactory
  friend class BehaviorFactory;
  explicit BehaviorOnboardingInterruptionHead(const Json::Value& config);  

  virtual void GetBehaviorOperationModifiers(BehaviorOperationModifiers& modifiers) const override;
  virtual void GetAllDelegates(std::set<IBehavior*>& delegates) const override {}
  virtual void GetBehaviorJsonKeys(std::set<const char*>& expectedKeys) const override;
  
  virtual bool WantsToBeActivatedBehavior() const override { return true; }
  virtual void OnBehaviorActivated() override;

private:

  struct InstanceConfig {
    InstanceConfig();
    AnimationTrigger animWhenGroggy;
  };

  struct DynamicVariables {
    DynamicVariables();
    bool isGroggy;
  };

  InstanceConfig _iConfig;
  DynamicVariables _dVars;
  
};

} // namespace Cozmo
} // namespace Anki

#endif // __Engine_AiComponent_BehaviorComponent_Behaviors_BehaviorOnboardingInterruptionHead__

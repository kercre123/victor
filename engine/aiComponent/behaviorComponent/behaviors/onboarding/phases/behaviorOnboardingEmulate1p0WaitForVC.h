/**
 * File: BehaviorOnboardingEmulate1p0WaitForVC.h
 *
 * Author: Sam Russell
 * Created: 2018-12-10
 *
 * Description: Reimplementing 1p0 onboarding in the new framework: sit still and await a VC, exit onboarding if one is recieved.
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#ifndef __Engine_AiComponent_BehaviorComponent_Behaviors_BehaviorOnboardingEmulate1p0WaitForVC__
#define __Engine_AiComponent_BehaviorComponent_Behaviors_BehaviorOnboardingEmulate1p0WaitForVC__
#pragma once

#include "engine/aiComponent/behaviorComponent/behaviors/iCozmoBehavior.h"

namespace Anki {
namespace Vector {

class BehaviorOnboardingEmulate1p0WaitForVC : public ICozmoBehavior
{
public: 
  virtual ~BehaviorOnboardingEmulate1p0WaitForVC();

protected:

  // Enforce creation through BehaviorFactory
  friend class BehaviorFactory;
  explicit BehaviorOnboardingEmulate1p0WaitForVC(const Json::Value& config);  

  virtual void GetBehaviorOperationModifiers(BehaviorOperationModifiers& modifiers) const override;
  virtual void GetAllDelegates(std::set<IBehavior*>& delegates) const override;
  virtual void GetBehaviorJsonKeys(std::set<const char*>& expectedKeys) const override {}
  
  virtual void InitBehavior() override;
  virtual bool WantsToBeActivatedBehavior() const override;
  virtual void OnBehaviorActivated() override;
  virtual void BehaviorUpdate() override;

private:

  struct InstanceConfig {
    InstanceConfig();
    ICozmoBehaviorPtr lookAtUserBehavior;
    ICozmoBehaviorPtr reactToTriggerWordBehavior;
  };

  struct DynamicVariables {
    DynamicVariables();
  };

  InstanceConfig _iConfig;
  DynamicVariables _dVars;
  
};

} // namespace Vector
} // namespace Anki

#endif // __Engine_AiComponent_BehaviorComponent_Behaviors_BehaviorOnboardingEmulate1p0WaitForVC__

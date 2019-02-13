/**
 * File: BehaviorOnboardingLookAtUser.h
 *
 * Author: Sam Russell
 * Created: 2018-11-05
 *
 * Description: Onboarding "Death Stare" implementation that handles On/Off charger state
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#ifndef __Engine_AiComponent_BehaviorComponent_Behaviors_BehaviorOnboardingLookAtUser__
#define __Engine_AiComponent_BehaviorComponent_Behaviors_BehaviorOnboardingLookAtUser__
#pragma once

#include "engine/aiComponent/behaviorComponent/behaviors/iCozmoBehavior.h"

namespace Anki {
namespace Vector {

class BehaviorOnboardingLookAtUser : public ICozmoBehavior
{
public: 
  virtual ~BehaviorOnboardingLookAtUser();

protected:

  // Enforce creation through BehaviorFactory
  friend class BehaviorFactory;
  explicit BehaviorOnboardingLookAtUser(const Json::Value& config);  

  virtual void GetBehaviorOperationModifiers(BehaviorOperationModifiers& modifiers) const override;
  virtual void GetAllDelegates(std::set<IBehavior*>& delegates) const override;
  virtual void GetBehaviorJsonKeys(std::set<const char*>& expectedKeys) const override {}
  
  virtual void InitBehavior() override;
  virtual bool WantsToBeActivatedBehavior() const override;
  virtual void OnBehaviorActivated() override;
  virtual void BehaviorUpdate() override;

private:

  void TransitionToLookAtUser();
  void TransitionToWaitingForPutDown();

  struct InstanceConfig {
    InstanceConfig();
    ICozmoBehaviorPtr behaviorLookAtUser;
    ICozmoBehaviorPtr behaviorLookAtUserOnCharger;
  };

  enum class LookAtUserState{
    LookAtUserOnCharger,
    LookAtUserOffCharger,
    WaitForPutDown
  };

  struct DynamicVariables {
    DynamicVariables();
    LookAtUserState state;
    ICozmoBehaviorPtr lastDelegate;
  };

  InstanceConfig _iConfig;
  DynamicVariables _dVars;
  
};

} // namespace Vector
} // namespace Anki

#endif // __Engine_AiComponent_BehaviorComponent_Behaviors_BehaviorOnboardingLookAtUser__

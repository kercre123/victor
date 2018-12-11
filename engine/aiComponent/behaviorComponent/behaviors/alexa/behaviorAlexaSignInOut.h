/**
 * File: behaviorAlexaSignInOut.h
 *
 * Author: ross
 * Created: 2018-12-02
 *
 * Description: handles commands to sign in and out of amazon
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#ifndef __Engine_AiComponent_BehaviorComponent_Behaviors_BehaviorAlexaSignInOut__
#define __Engine_AiComponent_BehaviorComponent_Behaviors_BehaviorAlexaSignInOut__
#pragma once

#include "engine/aiComponent/behaviorComponent/behaviors/iCozmoBehavior.h"
#include "engine/aiComponent/behaviorComponent/behaviorTreeStateHelpers.h"

namespace Anki {
namespace Vector {

class BehaviorAlexaSignInOut : public ICozmoBehavior
{
public: 
  virtual ~BehaviorAlexaSignInOut() = default;

protected:

  // Enforce creation through BehaviorFactory
  friend class BehaviorFactory;
  explicit BehaviorAlexaSignInOut(const Json::Value& config);  

  virtual void GetBehaviorOperationModifiers(BehaviorOperationModifiers& modifiers) const override;
  virtual void GetAllDelegates(std::set<IBehavior*>& delegates) const override;
  virtual void GetBehaviorJsonKeys(std::set<const char*>& expectedKeys) const override {}
  
  virtual void InitBehavior() override;
  virtual bool WantsToBeActivatedBehavior() const override;
  virtual void OnBehaviorActivated() override;
  virtual void OnBehaviorDeactivated() override;
  virtual void BehaviorUpdate() override;

private:
  
  void SignInOrOut(); // if not already signed in or out
  
  void PlaySignOutAnimAndExit();

  struct InstanceConfig {
    InstanceConfig();
    ICozmoBehaviorPtr lookAtFaceOrUpBehavior;
    AreBehaviorsActivatedHelper wakeWordBehaviors;
  };

  struct DynamicVariables {
    DynamicVariables();
    bool signingIn = false;
    float exitTimeout_s = -1.0f; // ignored if neg
    bool handled = false;
  };

  InstanceConfig _iConfig;
  DynamicVariables _dVars;
  
};

} // namespace Vector
} // namespace Anki

#endif // __Engine_AiComponent_BehaviorComponent_Behaviors_BehaviorAlexaSignInOut__

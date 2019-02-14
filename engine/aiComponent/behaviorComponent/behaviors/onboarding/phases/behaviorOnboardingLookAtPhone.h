/**
 * File: BehaviorOnboardingLookAtPhone.h
 *
 * Author: Sam
 * Created: 2018-06-27
 *
 * Description: keeps the head up while displaying a look at phone animation
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#ifndef __Engine_AiComponent_BehaviorComponent_Behaviors_BehaviorOnboardingLookAtPhone__
#define __Engine_AiComponent_BehaviorComponent_Behaviors_BehaviorOnboardingLookAtPhone__
#pragma once

#include "engine/aiComponent/behaviorComponent/behaviors/iCozmoBehavior.h"

namespace Anki {

namespace Util {
  class IConsoleFunction;
}

namespace Vector {

class BehaviorOnboardingLookAtPhone : public ICozmoBehavior
{
public:
  virtual ~BehaviorOnboardingLookAtPhone() = default;

protected:

  // Enforce creation through BehaviorFactory
  friend class BehaviorFactory;
  explicit BehaviorOnboardingLookAtPhone(const Json::Value& config);

  virtual void GetBehaviorOperationModifiers(BehaviorOperationModifiers& modifiers) const override;
  virtual void GetBehaviorJsonKeys(std::set<const char*>& expectedKeys) const override {}

  virtual void InitBehavior() override;

  virtual bool WantsToBeActivatedBehavior() const override;
  virtual void OnBehaviorActivated() override;
  virtual void BehaviorUpdate() override;
  virtual void AlwaysHandleInScope(const GameToEngineEvent& event) override;
  virtual void HandleWhileInScopeButNotActivated(const GameToEngineEvent& event) override;

private:

  void MoveHeadUp();
  void RunLoopAction();
  void HandleGameToEngineEvent(const GameToEngineEvent& event);

  struct InstanceConfig {
    InstanceConfig();
  };

  struct DynamicVariables {
    DynamicVariables();
    bool hasRun;
    bool receivedMessage;
  };

  InstanceConfig _iConfig;
  DynamicVariables _dVars;
  bool _hasBleKeys = true;

};

} // namespace Vector
} // namespace Anki

#endif // __Engine_AiComponent_BehaviorComponent_Behaviors_BehaviorOnboardingLookAtPhone__

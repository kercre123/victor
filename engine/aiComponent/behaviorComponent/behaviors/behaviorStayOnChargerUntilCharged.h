/**
 * File: BehaviorStayOnChargerUntilCharged.h
 *
 * Author: Andrew Stout
 * Created: 2019-01-31
 *
 * Description: WantsToBeActivated if on the charger and battery is not full; delegates to a parameterized behavior in the meantime.
 *
 * Copyright: Anki, Inc. 2019
 *
 **/

#ifndef __Engine_AiComponent_BehaviorComponent_Behaviors_BehaviorStayOnChargerUntilCharged__
#define __Engine_AiComponent_BehaviorComponent_Behaviors_BehaviorStayOnChargerUntilCharged__
#pragma once

#include "engine/aiComponent/behaviorComponent/behaviors/iCozmoBehavior.h"

namespace Anki {
namespace Vector {

class BehaviorStayOnChargerUntilCharged : public ICozmoBehavior
{
public: 
  virtual ~BehaviorStayOnChargerUntilCharged();

protected:

  // Enforce creation through BehaviorFactory
  friend class BehaviorFactory;
  explicit BehaviorStayOnChargerUntilCharged(const Json::Value& config);

  virtual void InitBehavior() override;
  virtual void GetBehaviorOperationModifiers(BehaviorOperationModifiers& modifiers) const override;
  virtual void GetAllDelegates(std::set<IBehavior*>& delegates) const override;
  virtual void GetBehaviorJsonKeys(std::set<const char*>& expectedKeys) const override;
  
  virtual bool WantsToBeActivatedBehavior() const override;
  virtual void OnBehaviorActivated() override;
  virtual void BehaviorUpdate() override;

private:

  struct InstanceConfig {
    InstanceConfig();
    std::string delegateName;
    ICozmoBehaviorPtr delegate;
  };

  struct DynamicVariables {
    DynamicVariables();
    float lastTimeCancelled_s;
  };

  InstanceConfig _iConfig;
  DynamicVariables _dVars;
  
};

} // namespace Vector
} // namespace Anki

#endif // __Engine_AiComponent_BehaviorComponent_Behaviors_BehaviorStayOnChargerUntilCharged__

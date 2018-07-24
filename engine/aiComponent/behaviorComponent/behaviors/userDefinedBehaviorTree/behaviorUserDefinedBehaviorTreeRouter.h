/**
 * File: BehaviorUserDefinedBehaviorTreeRouter.h
 *
 * Author: Hamzah Khan
 * Created: 2018-07-11
 *
 * Description: A behavior that uses the user-defined behavior component to delegate to the user-defined behavior, given a condition.
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#ifndef __Engine_AiComponent_BehaviorComponent_Behaviors_BehaviorUserDefinedBehaviorTreeRouter__
#define __Engine_AiComponent_BehaviorComponent_Behaviors_BehaviorUserDefinedBehaviorTreeRouter__
#pragma once

#include "engine/aiComponent/behaviorComponent/behaviors/iCozmoBehavior.h"
#include "engine/aiComponent/behaviorComponent/userDefinedBehaviorTreeComponent/userDefinedBehaviorTreeComponent.h"


namespace Anki {
namespace Cozmo {

class BehaviorUserDefinedBehaviorTreeRouter : public ICozmoBehavior
{
public: 
  virtual ~BehaviorUserDefinedBehaviorTreeRouter();

protected:

  // Enforce creation through BehaviorFactory
  friend class BehaviorFactory;
  explicit BehaviorUserDefinedBehaviorTreeRouter(const Json::Value& config);  

  virtual void GetBehaviorOperationModifiers(BehaviorOperationModifiers& modifiers) const override {};
  virtual void GetAllDelegates(std::set<IBehavior*>& delegates) const override final;
  virtual void GetBehaviorJsonKeys(std::set<const char*>& expectedKeys) const override;

  virtual void InitBehavior() override; 
  virtual bool WantsToBeActivatedBehavior() const override;
  virtual void OnBehaviorActivated() override;

private:

  struct InstanceConfig {
    InstanceConfig();
      std::set<IBEIConditionPtr> customizableConditions;
  };

  struct DynamicVariables {
    DynamicVariables();
  };

  InstanceConfig _iConfig;
  DynamicVariables _dVars;
  
};

} // namespace Cozmo
} // namespace Anki

#endif // __Engine_AiComponent_BehaviorComponent_Behaviors_BehaviorUserDefinedBehaviorTreeRouter__

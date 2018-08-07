/**
 * File: BehaviorPRDemoBase.h
 *
 * Author: Brad
 * Created: 2018-07-06
 *
 * Description: Base behavior for running the PR demo
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#ifndef __Engine_AiComponent_BehaviorComponent_Behaviors_BehaviorPRDemoBase__
#define __Engine_AiComponent_BehaviorComponent_Behaviors_BehaviorPRDemoBase__
#pragma once

#include "engine/aiComponent/behaviorComponent/behaviors/iCozmoBehavior.h"

namespace Anki {
namespace Vector {

class BehaviorPRDemo;

class BehaviorPRDemoBase : public ICozmoBehavior
{
public: 
  virtual ~BehaviorPRDemoBase();

protected:

  // Enforce creation through BehaviorFactory
  friend class BehaviorFactory;
  explicit BehaviorPRDemoBase(const Json::Value& config);  

  void GetAllDelegates(std::set<IBehavior*>& delegates) const override;
  void GetBehaviorOperationModifiers(BehaviorOperationModifiers& modifiers) const override;
  void GetBehaviorJsonKeys(std::set<const char*>& expectedKeys) const override {}

  void InitBehavior() override;
  bool WantsToBeActivatedBehavior() const override { return true; }
  void BehaviorUpdate() override;

  void OnBehaviorActivated() override;
  
private:

  struct InstanceConfig {
    InstanceConfig();
    // TODO: put configuration variables here
    std::shared_ptr<BehaviorPRDemo> demoBehavior;
    ICozmoBehaviorPtr sleepingBehavior;
    ICozmoBehaviorPtr wakeUpBehavior;
    ICozmoBehaviorPtr mainBehavior;
  };

  struct DynamicVariables {
    DynamicVariables();
  };

  InstanceConfig _iConfig;
  DynamicVariables _dVars;
  
};

} // namespace Vector
} // namespace Anki

#endif // __Engine_AiComponent_BehaviorComponent_Behaviors_BehaviorPRDemoBase__

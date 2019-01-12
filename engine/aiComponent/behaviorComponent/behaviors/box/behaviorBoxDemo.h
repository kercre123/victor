/**
 * File: BehaviorBoxDemo.h
 *
 * Author: Brad
 * Created: 2019-01-03
 *
 * Description: Main state machine for the box demo
 *
 * Copyright: Anki, Inc. 2019
 *
 **/

#ifndef __Engine_AiComponent_BehaviorComponent_Behaviors_BehaviorBoxDemo__
#define __Engine_AiComponent_BehaviorComponent_Behaviors_BehaviorBoxDemo__
#pragma once

#include "engine/aiComponent/behaviorComponent/behaviors/internalStatesBehavior.h"

namespace Anki {
namespace Vector {

class BehaviorBoxDemo : public InternalStatesBehavior
{
public: 
  virtual ~BehaviorBoxDemo();

protected:

  // Enforce creation through BehaviorFactory
  friend class BehaviorFactory;
  explicit BehaviorBoxDemo(const Json::Value& config);

  virtual bool WantsToBeActivatedBehavior() const override { return true; }

  virtual void GetBehaviorOperationModifiers(BehaviorOperationModifiers& modifiers) const override{
    modifiers.wantsToBeActivatedWhenCarryingObject = true;
    modifiers.wantsToBeActivatedWhenOffTreads = true;
    modifiers.wantsToBeActivatedWhenOnCharger = true;
    modifiers.behaviorAlwaysDelegates = false;
  }

  virtual void OnBehaviorActivatedInternal() override;
  virtual void OnBehaviorDeactivatedInternal() override;

private:

  struct InstanceConfig {
    InstanceConfig();
    // TODO: put configuration variables here
  };

  struct DynamicVariables {
    DynamicVariables();
    // TODO: put member variables here
  };

  InstanceConfig _iConfig;
  DynamicVariables _dVars;

  CustomBEIConditionHandleList CreateCustomConditions();
};

} // namespace Vector
} // namespace Anki

#endif // __Engine_AiComponent_BehaviorComponent_Behaviors_BehaviorBoxDemo__

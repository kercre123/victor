/**
 * File: BehaviorDispatcherRandomMap.h
 *
 * Author: Hamzah Khan
 * Created: 2018-05-24
 *
 * Description: A dispatcher that, upon initialization, maps a set of conditions to a set of behaviors. Upon activation by a condition, the corresponding behavior will be set.
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#ifndef __Engine_AiComponent_BehaviorComponent_Behaviors_BehaviorDispatcherRandomMap__
#define __Engine_AiComponent_BehaviorComponent_Behaviors_BehaviorDispatcherRandomMap__
#pragma once

#include "engine/aiComponent/behaviorComponent/behaviors/dispatch/iBehaviorDispatcher.h"
#include "engine/aiComponent/behaviorComponent/behaviors/iCozmoBehavior.h"

namespace Anki {
namespace Cozmo {

class BehaviorDispatcherRandomMap : public IBehaviorDispatcher
{
  using BaseClass = IBehaviorDispatcher;

  // Enforce creation through BehaviorFactory
  friend class BehaviorFactory;  
  BehaviorDispatcherRandomMap(const Json::Value& config);

protected:
  
  virtual void InitDispatcher() override;
  virtual void GetBehaviorJsonKeys(std::set<const char*>& expectedKeys) const override;
  virtual ICozmoBehaviorPtr GetDesiredBehavior() override;
  virtual void BehaviorDispatcher_OnActivated() override;
  virtual void BehaviorDispatcher_OnDeactivated() override;
  virtual void DispatcherUpdate() override;


private:
  struct InstanceConfig {
    InstanceConfig();
    // a vector mapping index of each condition to the index of each behavior
    std::vector< size_t > mapping;
    std::vector< IBEIConditionPtr > conditions;
    // // std::vector< BehaviorCooldownInfo > cooldownInfo;
  };

  struct DynamicVariables {
    DynamicVariables();
    bool shouldEndAfterBehavior;
    // // keep track of which behavior we last requested so that we can properly start the cooldown when the
    // // behavior ends
    // size_t lastDesiredBehaviorIdx;
  };

  InstanceConfig   _iConfig;
  DynamicVariables _dVars;

};

} // namespace Cozmo
} // namespace Anki

#endif // __Engine_AiComponent_BehaviorComponent_Behaviors_Dispatch_BehaviorDispatcherRandomMap__

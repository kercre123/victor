/**
 * File: behaviorDispatcherStrictPriorityWithCooldown.h
 *
 * Author: Brad Neuman
 * Created: 2017-10-24
 *
 * Description: Simple dispatcher similar to strict priority, but also supports cooldowns on behaviors
 *
 * Copyright: Anki, Inc. 2017
 *
 **/

#ifndef __Engine_AiComponent_BehaviorComponent_Behaviors_Dispatch_BehaviorDispatcherStrictPriorityWithCooldown_H__
#define __Engine_AiComponent_BehaviorComponent_Behaviors_Dispatch_BehaviorDispatcherStrictPriorityWithCooldown_H__

#include "engine/aiComponent/behaviorComponent/behaviors/dispatch/iBehaviorDispatcher.h"

#include "engine/aiComponent/behaviorComponent/behaviors/dispatch/helpers/behaviorCooldownInfo.h"

namespace Anki {
namespace Vector {

class BehaviorDispatcherStrictPriorityWithCooldown : public IBehaviorDispatcher
{
  using BaseClass = IBehaviorDispatcher;
  
  // Enforce creation through BehaviorFactory
  friend class BehaviorFactory;  
  BehaviorDispatcherStrictPriorityWithCooldown(const Json::Value& config);

protected:
  
  virtual void GetBehaviorJsonKeys(std::set<const char*>& expectedKeys) const override;
  virtual void GetLinkedActivatableScopeBehaviors(std::set<IBehavior*>& delegates) const override;
  virtual bool WantsToBeActivatedBehavior() const override;

  virtual ICozmoBehaviorPtr GetDesiredBehavior() override;
  virtual void BehaviorDispatcher_OnActivated() override;
  virtual void BehaviorDispatcher_OnDeactivated() override;

  virtual void DispatcherUpdate() override;

private:
  struct InstanceConfig {
    InstanceConfig();
    // index here matches the index in IBehaviorDispatcher::GetAllPossibleDispatches()
    std::vector< BehaviorCooldownInfo > cooldownInfo;

    // if true, links activation scope and WantsToBeActivated with it's delegates
    bool linkScope;
    
    // if true, this deactivating this behavior clears all cooldown info
    bool resetCooldownOnDeactivation;
  };

  struct DynamicVariables {
    DynamicVariables();
    // keep track of which behavior we last requested so that we can properly start the cooldown when the
    // behavior ends
    size_t lastDesiredBehaviorIdx;
  };

  InstanceConfig   _iConfig;
  DynamicVariables _dVars;
  
};

} // namespace Vector
} // namespace Anki


#endif // __Engine_AiComponent_BehaviorComponent_Behaviors_Dispatch_BehaviorDispatcherStrictPriorityWithCooldown_H__

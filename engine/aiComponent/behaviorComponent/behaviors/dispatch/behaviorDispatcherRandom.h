/**
 * File: behaviorDispatcherRandom.h
 *
 * Author: Brad Neuman
 * Created: 2017-10-31
 *
 * Description: Dispatcher which runs behaviors randomly based on weights and cooldowns. Once the delegated-to behavior
 *              ends, this behavior will cancel itself (this should probably be configurable - see VIC-4836)
 *
 * Copyright: Anki, Inc. 2017
 *
 **/

#ifndef __Engine_AiComponent_BehaviorComponent_Behaviors_Dispatch_BehaviorDispatcherRandom_H__
#define __Engine_AiComponent_BehaviorComponent_Behaviors_Dispatch_BehaviorDispatcherRandom_H__

#include "engine/aiComponent/behaviorComponent/behaviors/dispatch/iBehaviorDispatcher.h"

#include "engine/aiComponent/behaviorComponent/behaviors/dispatch/helpers/behaviorCooldownInfo.h"

namespace Anki {
namespace Vector {

class BehaviorDispatcherRandom : public IBehaviorDispatcher
{
  using BaseClass = IBehaviorDispatcher;

  // Enforce creation through BehaviorFactory
  friend class BehaviorFactory;  
  BehaviorDispatcherRandom(const Json::Value& config);

protected:

  virtual void GetBehaviorJsonKeys(std::set<const char*>& expectedKeys) const override;
  virtual ICozmoBehaviorPtr GetDesiredBehavior() override;
  virtual void BehaviorDispatcher_OnActivated() override;
  virtual void BehaviorDispatcher_OnDeactivated() override;
  virtual void DispatcherUpdate() override;

private:
  struct InstanceConfig {
    InstanceConfig();
    // index here matches the index in IBehaviorDispatcher::GetAllPossibleDispatches()
    std::vector< float > weights;
    std::vector< BehaviorCooldownInfo > cooldownInfo;
  };

  struct DynamicVariables {
    DynamicVariables();
    bool shouldEndAfterBehavior;
    // keep track of which behavior we last requested so that we can properly start the cooldown when the
    // behavior ends
    size_t lastDesiredBehaviorIdx;
  };

  InstanceConfig   _iConfig;
  DynamicVariables _dVars;

};

} // namespace Vector
} // namespace Anki

#endif // __Engine_AiComponent_BehaviorComponent_Behaviors_Dispatch_BehaviorDispatcherRandom_H__

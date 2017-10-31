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

namespace Anki {
namespace Cozmo {

class BehaviorDispatcherStrictPriorityWithCooldown : public IBehaviorDispatcher
{
  using BaseClass = IBehaviorDispatcher;
  
  // Enforce creation through BehaviorContainer
  friend class BehaviorContainer;  
  BehaviorDispatcherStrictPriorityWithCooldown(const Json::Value& config);

protected:
  
  virtual ICozmoBehaviorPtr GetDesiredBehavior(BehaviorExternalInterface& behaviorExternalInterface) override;
  virtual void BehaviorDispatcher_OnActivated(BehaviorExternalInterface& behaviorExternalInterface) override;
  virtual void BehaviorDispatcher_OnDeactivated(BehaviorExternalInterface& behaviorExternalInterface) override;

  virtual void BehaviorUpdate(BehaviorExternalInterface& behaviorExternalInterface) override;

private:

  struct CooldownInfo {
    bool OnCooldown() const;
    void StartCooldown(Util::RandomGenerator& rng);
    
    // params
    float _cooldown_s = 0.0f;;
    float _randomCooldownFactor = 0.0f;

    // member
    float _onCooldownUntil_s = -1.0f;
  };

  // index here matches the index in IBehaviorDispatcher::GetAllPossibleDispatches()
  std::vector< CooldownInfo > _cooldownInfo;

  // keep track of which behavior we last requested so that we can properly start the cooldown when the
  // behavior ends
  size_t _lastDesiredBehaviorIdx = 0;
  
};

}
}


#endif

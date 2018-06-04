/**
 * File: behaviorDispatcherStrictPriority.h
 *
 * Author: Brad Neuman
 * Created: 2017-10-13
 *
 * Description: Simple behavior which takes a json-defined list of other behaviors and dispatches to the first
 *              on in that list that wants to be activated
 *
 * Copyright: Anki, Inc. 2017
 *
 **/

#ifndef __Engine_AiComponent_BehaviorComponent_Behaviors_Dispatch_BehaviorDispatcherStrictPriority_H__
#define __Engine_AiComponent_BehaviorComponent_Behaviors_Dispatch_BehaviorDispatcherStrictPriority_H__

#include "engine/aiComponent/behaviorComponent/behaviors/dispatch/iBehaviorDispatcher.h"

namespace Anki {
namespace Cozmo {

class BehaviorDispatcherStrictPriority : public IBehaviorDispatcher
{
  using BaseClass = IBehaviorDispatcher;
  
  // Enforce creation through BehaviorFactory
  friend class BehaviorFactory;  
  BehaviorDispatcherStrictPriority(const Json::Value& config);

protected:
  virtual void BehaviorDispatcher_OnActivated() override;

  virtual ICozmoBehaviorPtr GetDesiredBehavior() override;
  virtual void GetBehaviorJsonKeys(std::set<const char*>& expectedKeys) const override;

  virtual void GetLinkedActivatableScopeBehaviors(std::set<IBehavior*>& delegates) const override;
  virtual bool WantsToBeActivatedBehavior() const override;
  
private:
  struct InstanceConfig {
    InstanceConfig();

    // if true, links activation scope and WantsToBeActivated with it's delegates
    bool linkScope;

    // if true, will CancelSelf after the FIRST behavior that WantsToBeActivated returns control
    bool actAsSelector;
  };

  struct DynamicVariables {
    DynamicVariables();

    // Track whether this behavior has yet delegated, in case it is acting as a selector.
    bool hasDelegated;
  };

  InstanceConfig   _iConfig;
  DynamicVariables _dVars;


};

} // namespace Cozmo
} // namespace Anki

#endif // __Engine_AiComponent_BehaviorComponent_Behaviors_Dispatch_BehaviorDispatcherStrictPriority_H__

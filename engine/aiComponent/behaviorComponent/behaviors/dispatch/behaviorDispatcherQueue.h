/**
 * File: behaviorDispatcherQueue.h
 *
 * Author: Brad Neuman
 * Created: 2017-10-30
 *
 * Description: Simple dispatch behavior which runs each behavior in a list in order, if it wants to be
 *              activated
 *
 * Copyright: Anki, Inc. 2017
 *
 **/

#ifndef __Engine_AiComponent_BehaviorComponent_Behaviors_Dispatch_BehaviorDispatcherQueue_H__
#define __Engine_AiComponent_BehaviorComponent_Behaviors_Dispatch_BehaviorDispatcherQueue_H__

#include "engine/aiComponent/behaviorComponent/behaviors/dispatch/iBehaviorDispatcher.h"

namespace Anki {
namespace Vector {

class BehaviorDispatcherQueue : public IBehaviorDispatcher
{
  // Enforce creation through BehaviorFactory
  friend class BehaviorFactory;  
  BehaviorDispatcherQueue(const Json::Value& config);

protected:
  
  virtual void GetBehaviorJsonKeys(std::set<const char*>& expectedKeys) const override;
  virtual ICozmoBehaviorPtr GetDesiredBehavior() override;
  virtual void BehaviorDispatcher_OnActivated() override;
  virtual void DispatcherUpdate() override;

private:
  struct InstanceConfig {
    InstanceConfig();
  };

  struct DynamicVariables {
    DynamicVariables();
    size_t currIdx = 0;
  };

  InstanceConfig   _iConfig;
  DynamicVariables _dVars;
};


} // namespace Vector
} // namespace Anki

#endif // __Engine_AiComponent_BehaviorComponent_Behaviors_Dispatch_BehaviorDispatcherQueue_H__

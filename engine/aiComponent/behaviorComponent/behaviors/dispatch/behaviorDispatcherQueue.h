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
namespace Cozmo {

class BehaviorDispatcherQueue : public IBehaviorDispatcher
{
  // Enforce creation through BehaviorContainer
  friend class BehaviorContainer;  
  BehaviorDispatcherQueue(const Json::Value& config);

protected:
  
  virtual ICozmoBehaviorPtr GetDesiredBehavior(BehaviorExternalInterface& behaviorExternalInterface) override;
  virtual void BehaviorDispatcher_OnActivated(BehaviorExternalInterface& behaviorExternalInterface) override;
  virtual void DispatcherUpdate(BehaviorExternalInterface& behaviorExternalInterface) override;

private:

  size_t _currIdx = 0;
};


}
}

#endif

/**
 * File: behaviorDispatcherRandom.h
 *
 * Author: Brad Neuman
 * Created: 2017-10-31
 *
 * Description: Dispatcher which runs behaviors randomly based on weights and cooldowns
 *
 * Copyright: Anki, Inc. 2017
 *
 **/

#ifndef __Engine_AiComponent_BehaviorComponent_Behaviors_Dispatch_BehaviorDispatcherRandom_H__
#define __Engine_AiComponent_BehaviorComponent_Behaviors_Dispatch_BehaviorDispatcherRandom_H__

#include "engine/aiComponent/behaviorComponent/behaviors/dispatch/iBehaviorDispatcher.h"

#include "engine/aiComponent/behaviorComponent/behaviors/dispatch/helpers/behaviorCooldownInfo.h"

namespace Anki {
namespace Cozmo {

class BehaviorDispatcherRandom : public IBehaviorDispatcher
{
  using BaseClass = IBehaviorDispatcher;

  // Enforce creation through BehaviorFactory
  friend class BehaviorFactory;  
  BehaviorDispatcherRandom(const Json::Value& config);

protected:

  virtual ICozmoBehaviorPtr GetDesiredBehavior() override;
  virtual void BehaviorDispatcher_OnActivated() override;
  virtual void DispatcherUpdate() override;

private:

  // index here matches the index in IBehaviorDispatcher::GetAllPossibleDispatches()
  std::vector< BehaviorCooldownInfo > _cooldownInfo;
  std::vector< float > _weights;

  bool _shouldEndAfterBehavior = false;
};

}
}

#endif

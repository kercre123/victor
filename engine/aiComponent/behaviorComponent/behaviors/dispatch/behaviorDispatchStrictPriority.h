/**
 * File: behaviorDispatchStrictPriority.h
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

#ifndef __Engine_AiComponent_BehaviorComponent_Behaviors_Dispatch_BehaviorDispatchStrictPriority_H__
#define __Engine_AiComponent_BehaviorComponent_Behaviors_Dispatch_BehaviorDispatchStrictPriority_H__

#include "engine/aiComponent/behaviorComponent/behaviors/iCozmoBehavior.h"

namespace Anki {
namespace Cozmo {

class BehaviorDispatchStrictPriority : public ICozmoBehavior
{
  // Enforce creation through BehaviorContainer
  friend class BehaviorContainer;  
  BehaviorDispatchStrictPriority(const Json::Value& config);

public:

  virtual bool WantsToBeActivatedBehavior(BehaviorExternalInterface& behaviorExternalInterface) const override;

  virtual bool CarryingObjectHandledInternally() const override;
  virtual bool ShouldRunWhileOffTreads() const override;
  virtual bool ShouldRunWhileOnCharger() const override;

  void GetAllDelegates(std::set<IBehavior*>& delegates) const override;
  
protected:

  virtual void InitBehavior(BehaviorExternalInterface& behaviorExternalInterface) override;
  virtual Result OnBehaviorActivated(BehaviorExternalInterface& behaviorExternalInterface) override;
  virtual Status UpdateInternal_WhileRunning(BehaviorExternalInterface& behaviorExternalInterface) override;

private:

  std::vector<BehaviorID> _behaviorIds;
  std::vector<ICozmoBehaviorPtr> _behaviors;
};

}
}

#endif

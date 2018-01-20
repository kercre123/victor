/**
 * File: iBehaviorDispatcher.h
 *
 * Author: Brad Neuman
 * Created: 2017-10-24
 *
 * Description: common interface for dispatch-type behaviors. These are behaviors that only dispatch to other
 *              behaviors
 *
 * Copyright: Anki, Inc. 2017
 *
 **/

#ifndef __Engine_AiComponent_BehaviorComponent_Behaviors_Dispatch_IBehaviorDispatcher_H__
#define __Engine_AiComponent_BehaviorComponent_Behaviors_Dispatch_IBehaviorDispatcher_H__

#include "engine/aiComponent/behaviorComponent/behaviors/iCozmoBehavior.h"

namespace Anki {
namespace Cozmo {

class IBehaviorDispatcher : public ICozmoBehavior
{
protected:
  IBehaviorDispatcher(const Json::Value& config);

  // second version of the constructor that allows child classes to force the value of
  // "interruptActiveBehavior". If not specified, the first constructor will read it from config
  IBehaviorDispatcher(const Json::Value& config, bool shouldInterruptActiveBehavior);

  // called during (child) constructors to add a behavior id that this behavior may dispatch to.
  void AddPossibleDispatch(BehaviorID id) { _behaviorIds.push_back(id); }

public:
  virtual bool WantsToBeActivatedBehavior() const override;
  virtual void GetAllDelegates(std::set<IBehavior*>& delegates) const final override;

protected:
  virtual void GetBehaviorOperationModifiers(BehaviorOperationModifiers& modifiers) const override;


  // This function should be overridden to return the behavior that should run. This will only be called at
  // "appropriate" times by the base class (the base class handles whether or not to interrupt running
  // behaviors)
  virtual ICozmoBehaviorPtr GetDesiredBehavior() = 0;

  // optional override for activated
  virtual void BehaviorDispatcher_OnActivated() {}
  virtual void BehaviorDispatcher_OnDeactivated() {}
  
  // behaviors will be returned in the order they were added
  const std::vector<ICozmoBehaviorPtr>& GetAllPossibleDispatches() const { return _behaviors; }

  // ICozmoBehavior functions:  
  virtual void InitBehavior() final override;
  virtual void InitDispatcher() {};  
  virtual void OnBehaviorActivated() final override;
  virtual void OnBehaviorDeactivated() final override;
  virtual void BehaviorUpdate() final override;
  virtual void DispatcherUpdate() {};
  virtual bool CanBeGentlyInterruptedNow() const final override;

private:

  std::vector<BehaviorID> _behaviorIds;
  std::vector<ICozmoBehaviorPtr> _behaviors;

  bool _shouldInterruptActiveBehavior;
};

}
}


#endif

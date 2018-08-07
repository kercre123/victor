/**
 * File: behaviorDispatcherRerun.h
 *
 * Author: Kevin M. Karol
 * Created: 10/24/17
 *
 * Description: A dispatcher which dispatches to its delegate the 
 * specified number of times before canceling itself - specifying -1 reruns
 * will cause the behavior to be dispatched infinitely
 *
 * Copyright: Anki, Inc. 2017
 *
 **/

#ifndef __Engine_AiComponent_BehaviorComponent_BehaviorDispatcherRerun_H__
#define __Engine_AiComponent_BehaviorComponent_BehaviorDispatcherRerun_H__

#include "engine/aiComponent/behaviorComponent/behaviors/iCozmoBehavior.h"
#include <vector>
#include <set>

namespace Anki {
namespace Vector {

class BehaviorDispatcherRerun : public ICozmoBehavior
{
public:
  virtual ~BehaviorDispatcherRerun();
  
  static Json::Value CreateConfig(BehaviorID newConfigID, BehaviorID delegateID, const int numRuns);  

  virtual bool WantsToBeActivatedBehavior() const override{return true;}
  
protected:
  // Construction has to go through BehvaiorContainer
  friend class BehaviorFactory;
  BehaviorDispatcherRerun(const Json::Value& config);

  virtual void GetBehaviorOperationModifiers(BehaviorOperationModifiers& modifiers) const override {
    modifiers.wantsToBeActivatedWhenCarryingObject = true;
    modifiers.wantsToBeActivatedWhenOffTreads = true;
    modifiers.wantsToBeActivatedWhenOnCharger = true;
    modifiers.behaviorAlwaysDelegates = false;
  }
  virtual void GetBehaviorJsonKeys(std::set<const char*>& expectedKeys) const override;

  virtual void InitBehavior() override;

  virtual void OnBehaviorActivated() override;
  virtual void BehaviorUpdate() override;
  virtual void OnBehaviorDeactivated() override;

  virtual void GetAllDelegates(std::set<IBehavior*>& delegates) const override;

private:
  struct InstanceConfig {
    InstanceConfig();
    int numRuns; 
    
    BehaviorID        delegateID;
    ICozmoBehaviorPtr delegatePtr;
  };

  struct DynamicVariables {
    DynamicVariables();
    int numRunsRemaining;
  };

  InstanceConfig   _iConfig;
  DynamicVariables _dVars;

  void CheckRerunState();
};

} // namespace Vector
} // namespace Anki

#endif // __Engine_AiComponent_BehaviorComponent_BehaviorDispatcherRerun_H__

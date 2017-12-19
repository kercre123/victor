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
namespace Cozmo {

class BehaviorDispatcherRerun : public ICozmoBehavior
{
public:
  virtual ~BehaviorDispatcherRerun();
  
  static Json::Value CreateConfig(BehaviorID newConfigID, BehaviorID delegateID, const int numRuns);  

  virtual bool WantsToBeActivatedBehavior(BehaviorExternalInterface& behaviorExternalInterface) const override{return true;}
  virtual bool CarryingObjectHandledInternally() const override{ return true;}
  virtual bool ShouldRunWhileOffTreads() const override{return true;}
  virtual bool ShouldRunWhileOnCharger() const override { return true;}
  
protected:
  // Construction has to go through BehvaiorContainer
  friend class BehaviorContainer;
  BehaviorDispatcherRerun(const Json::Value& config);

  virtual void InitBehavior(BehaviorExternalInterface& behaviorExternalInterface) override;
  virtual bool ShouldCancelWhenInControl() const override { return false;}

  virtual void OnBehaviorActivated(BehaviorExternalInterface& behaviorExternalInterface) override;
  virtual void BehaviorUpdate(BehaviorExternalInterface& behaviorExternalInterface) override;
  virtual void OnBehaviorDeactivated(BehaviorExternalInterface& behaviorExternalInterface) override;

  virtual void GetAllDelegates(std::set<IBehavior*>& delegates) const override;

private:
  struct Params{
    BehaviorID _delegateID;
    int _numRuns; 
  };
  Params _params;
  int _numRunsRemaining;
  ICozmoBehaviorPtr _delegatePtr;

  void CheckRerunState(BehaviorExternalInterface& behaviorExternalInterface);
};

}
}

#endif

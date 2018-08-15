/**
 * File: behaviorWait.h
 *
 * Author: Lee Crippen
 * Created: 10/01/15
 *
 * Description: Behavior to do nothing.
 *
 * Copyright: Anki, Inc. 2015
 *
 **/

#ifndef __Cozmo_Basestation_Behaviors_BehaviorWait_H__
#define __Cozmo_Basestation_Behaviors_BehaviorWait_H__

#include "engine/aiComponent/behaviorComponent/behaviors/iCozmoBehavior.h"

namespace Anki {
namespace Vector {
  
class BehaviorWait: public ICozmoBehavior
{
private:
  
  // Enforce creation through BehaviorFactory
  friend class BehaviorFactory;
  BehaviorWait(const Json::Value& config)
  : ICozmoBehavior(config)
  {
  }
  
public:
  
  virtual ~BehaviorWait() { }
  
  //
  // Abstract methods to be overloaded:
  //
  virtual bool WantsToBeActivatedBehavior() const override { return true; }


  virtual bool CanBeGentlyInterruptedNow() const override {
    return true; }

protected:
  virtual void GetBehaviorOperationModifiers(BehaviorOperationModifiers& modifiers) const override {
    modifiers.wantsToBeActivatedWhenCarryingObject = true;
    modifiers.wantsToBeActivatedWhenOffTreads = true;
    modifiers.wantsToBeActivatedWhenOnCharger = true;
    modifiers.behaviorAlwaysDelegates = false;
  }
  virtual void GetBehaviorJsonKeys(std::set<const char*>& expectedKeys) const override{}


  virtual void OnBehaviorActivated() override
  {
    
  }

  virtual void OnBehaviorDeactivated() override
  {
  }
};
  

} // namespace Vector
} // namespace Anki

#endif // __Cozmo_Basestation_Behaviors_BehaviorWait_H__

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
namespace Cozmo {
  
class BehaviorWait: public ICozmoBehavior
{
private:
  
  // Enforce creation through BehaviorContainer
  friend class BehaviorContainer;
  BehaviorWait(const Json::Value& config)
  : ICozmoBehavior(config)
  {
  }
  
public:
  
  virtual ~BehaviorWait() { }
  
  //
  // Abstract methods to be overloaded:
  //
  virtual bool WantsToBeActivatedBehavior(BehaviorExternalInterface& behaviorExternalInterface) const override { return true; }
  virtual bool CarryingObjectHandledInternally() const override { return true;}
  virtual bool ShouldRunWhileOffTreads() const override { return true;}

  
protected:
  
  virtual Result OnBehaviorActivated(BehaviorExternalInterface& behaviorExternalInterface) override
  {
    return Result::RESULT_OK;
  }
  
  virtual Status UpdateInternal_WhileRunning(BehaviorExternalInterface& behaviorExternalInterface) override
  {
    return Status::Running;
  }

  virtual void OnBehaviorDeactivated(BehaviorExternalInterface& behaviorExternalInterface) override
  {
  }
};
  

} // namespace Cozmo
} // namespace Anki

#endif // __Cozmo_Basestation_Behaviors_BehaviorWait_H__

/**
 * File: behaviorReactToReturnedToTreads.h
 *
 * Author: Kevin M. Karol
 * Created: 2016-08-24
 *
 * Description: Cozmo reacts to being placed back on his treads (cancels playing animations)
 *
 * Copyright: Anki, Inc. 2016
 *
 **/

#ifndef __Cozmo_Basestation_Behaviors_BeahviorReactToReturnedToTreads_H__
#define __Cozmo_Basestation_Behaviors_BeahviorReactToReturnedToTreads_H__

#include "engine/aiComponent/behaviorComponent/behaviors/iCozmoBehavior.h"

namespace Anki {
namespace Cozmo {

class BehaviorReactToReturnedToTreads : public ICozmoBehavior
{
private:
  
  // Enforce creation through BehaviorContainer
  friend class BehaviorContainer;
  BehaviorReactToReturnedToTreads(const Json::Value& config);

public:
  
  virtual bool WantsToBeActivatedBehavior(BehaviorExternalInterface& behaviorExternalInterface) const override;
  virtual bool ShouldRunWhileOffTreads() const override { return true;}
  
  virtual bool CarryingObjectHandledInternally() const override {return true;}
  
protected:
  virtual void OnBehaviorActivated(BehaviorExternalInterface& behaviorExternalInterface) override;
  virtual void OnBehaviorDeactivated(BehaviorExternalInterface& behaviorExternalInterface) override;
  

private:
  void CheckForHighPitch(BehaviorExternalInterface& behaviorExternalInterface);
};

}
}

#endif

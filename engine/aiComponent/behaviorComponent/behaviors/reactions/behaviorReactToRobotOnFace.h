/**
 * File: behaviorReactToRobotOnFace.h
 *
 * Author: Kevin M. Karol
 * Created: 2016-07-18
 *
 * Description: 
 *
 * Copyright: Anki, Inc. 2016
 *
 **/

#ifndef __Cozmo_Basestation_Behaviors_BeahviorReactToRobotOnFace_H__
#define __Cozmo_Basestation_Behaviors_BeahviorReactToRobotOnFace_H__

#include "engine/aiComponent/behaviorComponent/behaviors/iBehavior.h"

namespace Anki {
namespace Cozmo {

class BehaviorReactToRobotOnFace : public IBehavior
{
private:
  // Enforce creation through BehaviorContainer
  friend class BehaviorContainer;
  BehaviorReactToRobotOnFace(const Json::Value& config);
  
public:
  virtual bool WantsToBeActivatedBehavior(BehaviorExternalInterface& behaviorExternalInterface) const override;
  virtual bool ShouldRunWhileOffTreads() const override { return true;}
  
  virtual bool CarryingObjectHandledInternally() const override {return true;}


protected:
  virtual Result OnBehaviorActivated(BehaviorExternalInterface& behaviorExternalInterface) override;
  virtual void   OnBehaviorDeactivated(BehaviorExternalInterface& behaviorExternalInterface) override;

private:
  void FlipOverIfNeeded(BehaviorExternalInterface& behaviorExternalInterface);
  void DelayThenCheckState(BehaviorExternalInterface& behaviorExternalInterface);
  void CheckFlipSuccess(BehaviorExternalInterface& behaviorExternalInterface);
  
  
};

}
}

#endif

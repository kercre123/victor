/**
 * File: behaviorReactToRobotOnBack.h
 *
 * Author: Brad Neuman
 * Created: 2016-05-06
 *
 * Description: 
 *
 * Copyright: Anki, Inc. 2016
 *
 **/

#ifndef __Cozmo_Basestation_Behaviors_BeahviorReactToRobotOnBack_H__
#define __Cozmo_Basestation_Behaviors_BeahviorReactToRobotOnBack_H__

#include "engine/aiComponent/behaviorSystem/behaviors/iBehavior.h"

namespace Anki {
namespace Cozmo {

class BehaviorReactToRobotOnBack : public IBehavior
{
private:
  
  // Enforce creation through BehaviorContainer
  friend class BehaviorContainer;
  BehaviorReactToRobotOnBack(const Json::Value& config);
  
public:
  virtual bool WantsToBeActivatedBehavior(BehaviorExternalInterface& behaviorExternalInterface) const override;
  // don't know where the robot will land, so don't resume
  // TODO:(bn) should this depend on how long the robot was "in the air"?
  virtual bool ShouldRunWhileOffTreads() const override { return true;}

  virtual bool CarryingObjectHandledInternally() const override {return true;}

protected:
  
  virtual Result OnBehaviorActivated(BehaviorExternalInterface& behaviorExternalInterface) override;
  virtual void   OnBehaviorDeactivated(BehaviorExternalInterface& behaviorExternalInterface) override;

private:

  void FlipDownIfNeeded(BehaviorExternalInterface& behaviorExternalInterface);
  void DelayThenFlipDown(BehaviorExternalInterface& behaviorExternalInterface);
  
};

}
}

#endif

/**
 * File: behaviorReactToRobotOnSide.h
 *
 * Author: Kevin M. Karol
 * Created: 2016-07-18
 *
 * Description: 
 *
 * Copyright: Anki, Inc. 2016
 *
 **/

#ifndef __Cozmo_Basestation_Behaviors_BeahviorReactToRobotOnSide_H__
#define __Cozmo_Basestation_Behaviors_BeahviorReactToRobotOnSide_H__

#include "engine/aiComponent/behaviorComponent/behaviors/iBehavior.h"

namespace Anki {
namespace Cozmo {

class BehaviorReactToRobotOnSide : public IBehavior
{
private:
  
  // Enforce creation through BehaviorContainer
  friend class BehaviorContainer;
  BehaviorReactToRobotOnSide(const Json::Value& config);
  
public:
  virtual bool WantsToBeActivatedBehavior(BehaviorExternalInterface& behaviorExternalInterface) const override;
  virtual bool ShouldRunWhileOffTreads() const override { return true;}  
  virtual bool CarryingObjectHandledInternally() const override {return true;}

  
protected:
    
  virtual Result OnBehaviorActivated(BehaviorExternalInterface& behaviorExternalInterface) override;
  virtual void   OnBehaviorDeactivated(BehaviorExternalInterface& behaviorExternalInterface) override;

private:

  void ReactToBeingOnSide(BehaviorExternalInterface& behaviorExternalInterface);
  void AskToBeRighted(BehaviorExternalInterface& behaviorExternalInterface);
  //Ensures no other behaviors run while Cozmo is still on his side
  void HoldingLoop(BehaviorExternalInterface& behaviorExternalInterface);

  float _timeToPerformBoredAnim_s = -1.0f;
  
};

}
}

#endif

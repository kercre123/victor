/**
 * File: behaviorReactToUnexpectedMovement.h
 *
 * Author: Al Chaussee
 * Created: 7/11/2016
 *
 * Description: Behavior for reacting to unexpected movement like being spun while moving
 *
 * Copyright: Anki, Inc. 2016
 *
 **/

#ifndef __Cozmo_Basestation_Behaviors_BehaviorReactToUnexpectedMovement_H__
#define __Cozmo_Basestation_Behaviors_BehaviorReactToUnexpectedMovement_H__

#include "engine/aiComponent/behaviorSystem/behaviors/iBehavior.h"
#include "clad/robotInterface/messageRobotToEngine.h"
#include "clad/types/unexpectedMovementTypes.h"
#include "util/signals/simpleSignal_fwd.h"

namespace Anki {
namespace Cozmo {

class Robot;
  
class BehaviorReactToUnexpectedMovement : public IBehavior
{
private:
  using super = IBehavior;
  
  friend class BehaviorContainer;
  BehaviorReactToUnexpectedMovement(const Json::Value& config);
  
  UnexpectedMovementSide _unexpectedMovementSide = UnexpectedMovementSide::UNKNOWN;
  
public:  
  virtual bool IsRunnableInternal(BehaviorExternalInterface& behaviorExternalInterface) const override;
  virtual bool CarryingObjectHandledInternally() const override {return true;}
  
protected:
  virtual Result OnBehaviorActivated(BehaviorExternalInterface& behaviorExternalInterface) override;
  virtual void   OnBehaviorDeactivated(BehaviorExternalInterface& behaviorExternalInterface) override { };
  virtual void   AlwaysHandle(const EngineToGameEvent& event, BehaviorExternalInterface& behaviorExternalInterface) override;
};
  
}
}

#endif // __Cozmo_Basestation_Behaviors_BehaviorReactToUnexpectedMovement_H__

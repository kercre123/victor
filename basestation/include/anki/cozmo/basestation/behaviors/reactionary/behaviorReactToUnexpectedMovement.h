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

#include "anki/cozmo/basestation/behaviors/behaviorInterface.h"
#include "clad/robotInterface/messageRobotToEngine.h"
#include "util/signals/simpleSignal_fwd.h"

namespace Anki {
namespace Cozmo {

class Robot;
  
class BehaviorReactToUnexpectedMovement : public IReactionaryBehavior
{
private:
  using super = IReactionaryBehavior;
  
  friend class BehaviorFactory;
  BehaviorReactToUnexpectedMovement(Robot& robot, const Json::Value& config);
  
public:
  
  virtual bool IsRunnableInternalReactionary(const Robot& robot) const override;
  virtual bool ShouldResumeLastBehavior() const override { return true; }
  virtual bool ShouldRunForEvent(const ExternalInterface::MessageEngineToGame& event, const Robot& robot) override;
  
protected:
  
  virtual Result InitInternalReactionary(Robot& robot) override;
  virtual void   StopInternalReactionary(Robot& robot) override { };

};
  
}
}

#endif // __Cozmo_Basestation_Behaviors_BehaviorReactToUnexpectedMovement_H__
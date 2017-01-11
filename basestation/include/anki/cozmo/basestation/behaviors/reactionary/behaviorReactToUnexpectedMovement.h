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

#include "anki/cozmo/basestation/behaviors/iBehavior.h"
#include "clad/robotInterface/messageRobotToEngine.h"
#include "util/signals/simpleSignal_fwd.h"

namespace Anki {
namespace Cozmo {

class Robot;
  
class BehaviorReactToUnexpectedMovement : public IBehavior
{
private:
  using super = IBehavior;
  
  friend class BehaviorFactory;
  BehaviorReactToUnexpectedMovement(Robot& robot, const Json::Value& config);
  
public:  
  virtual bool IsRunnableInternal(const BehaviorPreReqNone& preReqData) const override;
  virtual bool CarryingObjectHandledInternally() const override {return true;}
  
protected:
  virtual Result InitInternal(Robot& robot) override;
  virtual void   StopInternal(Robot& robot) override { };
};
  
}
}

#endif // __Cozmo_Basestation_Behaviors_BehaviorReactToUnexpectedMovement_H__
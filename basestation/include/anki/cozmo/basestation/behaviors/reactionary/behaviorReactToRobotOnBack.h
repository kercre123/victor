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

#include "anki/cozmo/basestation/behaviors/behaviorInterface.h"

namespace Anki {
namespace Cozmo {

class BehaviorReactToRobotOnBack : public IReactionaryBehavior
{
private:
  
  // Enforce creation through BehaviorFactory
  friend class BehaviorFactory;
  BehaviorReactToRobotOnBack(Robot& robot, const Json::Value& config);
  
public:
  
  virtual bool IsRunnableInternalReactionary(const Robot& robot) const override;
  // don't know where the robot will land, so don't resume
  // TODO:(bn) should this depend on how long the robot was "in the air"?
  virtual bool ShouldResumeLastBehavior() const override { return false; }
  virtual bool ShouldRunWhileOffTreads() const override { return true;}
  virtual bool ShouldComputationallySwitch(const Robot& robot) override;

  
protected:
  
  virtual Result InitInternalReactionary(Robot& robot) override;
  virtual void   StopInternalReactionary(Robot& robot) override;

private:

  void FlipDownIfNeeded(Robot& robot);
  void DelayThenFlipDown(Robot& robot);
  
};

}
}

#endif

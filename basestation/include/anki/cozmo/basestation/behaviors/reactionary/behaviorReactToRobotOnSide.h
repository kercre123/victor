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

#include "anki/cozmo/basestation/behaviors/behaviorInterface.h"

namespace Anki {
namespace Cozmo {

class BehaviorReactToRobotOnSide : public IReactionaryBehavior
{
private:
  
  // Enforce creation through BehaviorFactory
  friend class BehaviorFactory;
  BehaviorReactToRobotOnSide(Robot& robot, const Json::Value& config);
  
public:
  
  virtual bool IsRunnableInternalReactionary(const Robot& robot) const override;
  virtual bool ShouldResumeLastBehavior() const override { return false; }
  virtual bool ShouldRunWhileOffTreads() const override { return true;}
  virtual bool ShouldComputationallySwitch(const Robot& robot) override;
  
protected:
    
  virtual Result InitInternalReactionary(Robot& robot) override;
  virtual void   StopInternalReactionary(Robot& robot) override;

private:

  void ReactToBeingOnSide(Robot& robot);
  void AskToBeRighted(Robot& robot);
  //Ensures no other behaviors run while Cozmo is still on his side
  void HoldingLoop(Robot& robot);
  
};

}
}

#endif

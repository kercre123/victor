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

#include "anki/cozmo/basestation/behaviors/behaviorInterface.h"

namespace Anki {
namespace Cozmo {

class BehaviorReactToRobotOnFace : public IReactionaryBehavior
{
private:
  
  // Enforce creation through BehaviorFactory
  friend class BehaviorFactory;
  BehaviorReactToRobotOnFace(Robot& robot, const Json::Value& config);
  
public:
  
  virtual bool IsRunnableInternalReactionary(const Robot& robot) const override;
  virtual bool ShouldResumeLastBehavior() const override { return false; }
  virtual bool ShouldRunForEvent(const ExternalInterface::MessageEngineToGame& event, const Robot& robot) override;
  
protected:
    
  virtual Result InitInternalReactionary(Robot& robot) override;
  virtual void   StopInternalReactionary(Robot& robot) override;

private:
  void FlipOverIfNeeded(Robot& robot);
  void DelayThenCheckState(Robot& robot);
  void CheckFlipSuccess(Robot& robot);
  
  
};

}
}

#endif

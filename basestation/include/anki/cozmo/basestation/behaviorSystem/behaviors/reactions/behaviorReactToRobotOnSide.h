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

#include "anki/cozmo/basestation/behaviorSystem/behaviors/iBehavior.h"

namespace Anki {
namespace Cozmo {

class BehaviorReactToRobotOnSide : public IBehavior
{
private:
  
  // Enforce creation through BehaviorFactory
  friend class BehaviorFactory;
  BehaviorReactToRobotOnSide(Robot& robot, const Json::Value& config);
  
public:
  virtual bool IsRunnableInternal(const BehaviorPreReqNone& preReqData) const override;
  virtual bool ShouldRunWhileOffTreads() const override { return true;}  
  virtual bool CarryingObjectHandledInternally() const override {return true;}

  
protected:
    
  virtual Result InitInternal(Robot& robot) override;
  virtual void   StopInternal(Robot& robot) override;

private:

  void ReactToBeingOnSide(Robot& robot);
  void AskToBeRighted(Robot& robot);
  //Ensures no other behaviors run while Cozmo is still on his side
  void HoldingLoop(Robot& robot);

  float _timeToPerformBoredAnim_s = -1.0f;
  
};

}
}

#endif

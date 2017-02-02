/**
 * File: behaviorReactToReturnedToTreads.h
 *
 * Author: Kevin M. Karol
 * Created: 2016-08-24
 *
 * Description: Cozmo reacts to being placed back on his treads (cancels playing animations)
 *
 * Copyright: Anki, Inc. 2016
 *
 **/

#ifndef __Cozmo_Basestation_Behaviors_BeahviorReactToReturnedToTreads_H__
#define __Cozmo_Basestation_Behaviors_BeahviorReactToReturnedToTreads_H__

#include "anki/cozmo/basestation/behaviors/iBehavior.h"

namespace Anki {
namespace Cozmo {

class BehaviorReactToReturnedToTreads : public IBehavior
{
private:
  
  // Enforce creation through BehaviorFactory
  friend class BehaviorFactory;
  BehaviorReactToReturnedToTreads(Robot& robot, const Json::Value& config);

public:
  
  virtual bool IsRunnableInternal(const BehaviorPreReqNone& preReqData) const override;
  virtual bool ShouldRunWhileOffTreads() const override { return true;}
  
  virtual bool CarryingObjectHandledInternally() const override {return true;}
  
protected:
  virtual Result InitInternal(Robot& robot) override;
  virtual void   StopInternal(Robot& robot) override;
  

private:
  void CheckForHighPitch(Robot& robot);
};

}
}

#endif

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

#include "anki/cozmo/basestation/behaviors/behaviorInterface.h"

namespace Anki {
namespace Cozmo {

class BehaviorReactToReturnedToTreads : public IReactionaryBehavior
{
private:
  
  // Enforce creation through BehaviorFactory
  friend class BehaviorFactory;
  BehaviorReactToReturnedToTreads(Robot& robot, const Json::Value& config);
  virtual bool ShouldRunForEvent(const ExternalInterface::MessageEngineToGame& event, const Robot& robot) override;

public:
  
  virtual bool IsRunnableInternalReactionary(const Robot& robot) const override;
  virtual bool ShouldResumeLastBehavior() const override { return false; }
  virtual bool ShouldRunWhileOffTreads() const override { return true;}
  
protected:
    
  virtual Result InitInternalReactionary(Robot& robot) override;
  virtual void   StopInternalReactionary(Robot& robot) override;

private:
  
  void CheckForHighPitch(Robot& robot);
};

}
}

#endif

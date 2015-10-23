/**
 * File: behaviorNone.h
 *
 * Author: Lee Crippen
 * Created: 10/01/15
 *
 * Description: Behavior to do nothing.
 *
 * Copyright: Anki, Inc. 2015
 *
 **/

#ifndef __Cozmo_Basestation_Behaviors_BehaviorNone_H__
#define __Cozmo_Basestation_Behaviors_BehaviorNone_H__

#include "anki/cozmo/basestation/behaviors/behaviorInterface.h"

namespace Anki {
namespace Cozmo {
  
class BehaviorNone: public IBehavior
{
public:
  BehaviorNone(Robot& robot, const Json::Value& config) : IBehavior(robot, config) { _name = "NoneBehavior"; }
  virtual ~BehaviorNone() { }
  
  //
  // Abstract methods to be overloaded:
  //
  virtual bool IsRunnable(double currentTime_sec) const override { return true; }
  
  virtual Result Interrupt(Robot& robot, double currentTime_sec) override
  {
    _isInterrupted = true; return Result::RESULT_OK;
  }
  
  virtual bool GetRewardBid(Reward& reward) override { return true; }
  
protected:
  
  virtual Result InitInternal(Robot& robot, double currentTime_sec) override
  {
    _isInterrupted = false; return Result::RESULT_OK;
  }
  
  virtual Status UpdateInternal(Robot& robot, double currentTime_sec) override
  {
    Status retval = _isInterrupted ? Status::Complete : Status::Running;
    return retval;
  }

  
  bool _isInterrupted = false;
};
  

} // namespace Cozmo
} // namespace Anki

#endif // __Cozmo_Basestation_Behaviors_BehaviorNone_H__

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
private:
  
  // Enforce creation through BehaviorFactory
  friend class BehaviorFactory;
  BehaviorNone(Robot& robot, const Json::Value& config) : IBehavior(robot, config)
  {
    SetDefaultName("NoneBehavior");
  }
  
public:
  
  virtual ~BehaviorNone() { }
  
  //
  // Abstract methods to be overloaded:
  //
  virtual bool IsRunnable(const Robot& robot) const override { return true; }
  
protected:
  
  virtual Result InitInternal(Robot& robot) override
  {
    _isInterrupted = false; return Result::RESULT_OK;
  }
  
  virtual Status UpdateInternal(Robot& robot) override
  {
    Status retval = _isInterrupted ? Status::Complete : Status::Running;
    return retval;
  }

  virtual Result InterruptInternal(Robot& robot) override
  {
    _isInterrupted = true; return Result::RESULT_OK;
  }

  virtual void StopInternal(Robot& robot) override
  {
  }
  
  bool _isInterrupted = false;
};
  

} // namespace Cozmo
} // namespace Anki

#endif // __Cozmo_Basestation_Behaviors_BehaviorNone_H__

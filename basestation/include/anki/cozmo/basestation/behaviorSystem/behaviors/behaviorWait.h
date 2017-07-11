/**
 * File: behaviorWait.h
 *
 * Author: Lee Crippen
 * Created: 10/01/15
 *
 * Description: Behavior to do nothing.
 *
 * Copyright: Anki, Inc. 2015
 *
 **/

#ifndef __Cozmo_Basestation_Behaviors_BehaviorWait_H__
#define __Cozmo_Basestation_Behaviors_BehaviorWait_H__

#include "anki/cozmo/basestation/behaviorSystem/behaviors/iBehavior.h"

namespace Anki {
namespace Cozmo {
  
class BehaviorWait: public IBehavior
{
private:
  
  // Enforce creation through BehaviorContainer
  friend class BehaviorContainer;
  BehaviorWait(Robot& robot, const Json::Value& config) : IBehavior(robot, config)
  {
  }
  
public:
  
  virtual ~BehaviorWait() { }
  
  //
  // Abstract methods to be overloaded:
  //
  virtual bool IsRunnableInternal(const BehaviorPreReqNone& preReqData) const override { return true; }
  virtual bool CarryingObjectHandledInternally() const override { return true;}
  virtual bool ShouldRunWhileOffTreads() const override { return true;}

  
protected:
  
  virtual Result InitInternal(Robot& robot) override
  {
    return Result::RESULT_OK;
  }
  
  virtual Status UpdateInternal(Robot& robot) override
  {
    return Status::Running;
  }

  virtual void StopInternal(Robot& robot) override
  {
  }
};
  

} // namespace Cozmo
} // namespace Anki

#endif // __Cozmo_Basestation_Behaviors_BehaviorWait_H__

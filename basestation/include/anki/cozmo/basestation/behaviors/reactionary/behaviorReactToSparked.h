/**
 * File: behaviorReactToSparked.h
 *
 * Author:  Kevin M. Karol
 * Created: 2016-09-13
 *
 * Description:  Reaction that cancels other reactions when cozmo is sparked
 *
 * Copyright: Anki, Inc. 2016
 *
 **/

#ifndef __Cozmo_Basestation_Behaviors_BehaviorReactToSparked_H__
#define __Cozmo_Basestation_Behaviors_BehaviorReactToSparked_H__

#include "anki/cozmo/basestation/behaviors/iBehavior.h"

namespace Anki {
namespace Cozmo {
  
class BehaviorReactToSparked : public IBehavior
{
public:
  virtual bool CarryingObjectHandledInternally() const override {return false;}
  
protected:
  // Enforce creation through BehaviorFactory
  friend class BehaviorFactory;
  BehaviorReactToSparked(Robot& robot, const Json::Value& config);

  virtual Result InitInternal(Robot& robot) override;
  virtual bool IsRunnableInternal(const BehaviorPreReqNone& preReqData) const override;
  virtual bool ShouldRunWhileOffTreads() const override { return true;}
  
}; // class BehaviorReactToSparked

  
} // namespace Cozmo
} // namespace Anki

#endif // __Cozmo_Basestation_Behaviors_BehaviorReactToSparked_H__

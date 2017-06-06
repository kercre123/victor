/**
 * File: behaviorReactToPyramid.h
 *
 * Author: Kevin M. Karol
 * Created: 2016-10-27
 *
 * Description: Cozmo reacts to seeing a pyramid
 *
 * Copyright: Anki, Inc. 2016
 *
 **/

#ifndef __Cozmo_Basestation_Behaviors_BehaviorReactToPyramid_H__
#define __Cozmo_Basestation_Behaviors_BehaviorReactToPyramid_H__

#include "anki/cozmo/basestation/behaviorSystem/behaviors/iBehavior.h"

namespace Anki {
namespace Cozmo {

class BehaviorReactToPyramid : public IBehavior
{
private:
  // Enforce creation through BehaviorFactory
  friend class BehaviorFactory;
  BehaviorReactToPyramid(Robot& robot, const Json::Value& config);

public:
  virtual bool IsRunnableInternal(const BehaviorPreReqRobot& preReqData) const override;
  virtual bool ShouldRunWhileOffTreads() const override { return false;}
  virtual bool CarryingObjectHandledInternally() const override {return false;}
  
protected:
  virtual Result InitInternal(Robot& robot) override;
  
private:
  float _nextValidReactionTime_s;
  
};

}// namespace Cozmo
}// namespace Anki

#endif

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

#include "anki/cozmo/basestation/behaviors/behaviorInterface.h"

namespace Anki {
namespace Cozmo {

class BehaviorReactToPyramid : public IReactionaryBehavior
{
private:
  // Enforce creation through BehaviorFactory
  friend class BehaviorFactory;
  BehaviorReactToPyramid(Robot& robot, const Json::Value& config);

public:
  virtual bool IsRunnableInternalReactionary(const Robot& robot) const override;
  virtual bool ShouldResumeLastBehavior() const override { return true; }
  virtual bool ShouldRunWhileOffTreads() const override { return false;}
  
protected:
  virtual Result InitInternalReactionary(Robot& robot) override;
  
private:
  TimeStamp_t _nextValidReactionTime_s;
  
};

}// namespace Cozmo
}// namespace Anki

#endif

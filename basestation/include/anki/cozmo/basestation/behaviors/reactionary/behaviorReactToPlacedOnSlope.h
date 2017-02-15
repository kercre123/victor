/**
 * File: behaviorReactToPlacedOnSlope.h
 *
 * Author: Matt Michini
 * Created: 01/25/17
 *
 * Description: Behavior for responding to robot being placed down on a slope or at a pitch angle away from horizontal.
 *
 * Copyright: Anki, Inc. 2017
 *
 **/

#ifndef __Cozmo_Basestation_Behaviors_behaviorReactToPlacedOnSlope_H__
#define __Cozmo_Basestation_Behaviors_behaviorReactToPlacedOnSlope_H__

#include "anki/cozmo/basestation/behaviors/iBehavior.h"

namespace Anki {
namespace Cozmo {
  
class BehaviorReactToPlacedOnSlope : public IBehavior
{
private:
  
  // Enforce creation through BehaviorFactory
  friend class BehaviorFactory;
  BehaviorReactToPlacedOnSlope(Robot& robot, const Json::Value& config);
  
public:
  virtual bool IsRunnableInternal(const BehaviorPreReqNone& preReqData) const override;
  virtual bool CarryingObjectHandledInternally() const override {return true;}

protected:
  virtual Result InitInternal(Robot& robot) override;
  virtual bool ShouldRunWhileOffTreads() const override { return true;}


private:

  // Check robot's pitch angle at the end of the behavior
  void CheckPitch(Robot& robot);
  
  // Keeps track of whether or not the robot ended the behavior still inclined
  bool _endedOnInclineLastTime = false;
  
  // The last time the behavior was run
  double _lastBehaviorTime = 0.0;
  
}; // class BehaviorReactToPlacedOnSlope
  

} // namespace Cozmo
} // namespace Anki

#endif // __Cozmo_Basestation_Behaviors_behaviorReactToPlacedOnSlope_H__

/**
 * File: behaviorFollowMotion.h
 *
 * Author: Andrew Stein
 * Created: 11/13/15
 *
 * Description: Behavior for following motion in the image
 *
 * Copyright: Anki, Inc. 2015
 *
 **/
#ifndef __Cozmo_Basestation_Behaviors_BehaviorFollowMotion_H__
#define __Cozmo_Basestation_Behaviors_BehaviorFollowMotion_H__

#include "anki/cozmo/basestation/behaviors/behaviorInterface.h"
#include "anki/common/basestation/math/point.h"
#include "anki/common/basestation/objectIDs.h"
#include "anki/common/shared/radians.h"

#include <vector>

namespace Anki {
namespace Cozmo {

class BehaviorFollowMotion : public IBehavior
{
private:
  
  // Enforce creation through BehaviorFactory
  friend class BehaviorFactory;
  BehaviorFollowMotion(Robot& robot, const Json::Value& config);
  
public:
  
  virtual bool IsRunnable(const Robot& robot, double currentTime_sec) const override;

  virtual float EvaluateScoreInternal(const Robot& robot, double currentTime_sec) const override;

protected:
    
  virtual Result InitInternal(Robot& robot, double currentTime_sec, bool isResuming) override;
  virtual Status UpdateInternal(Robot& robot, double currentTime_sec) override;
  virtual Result InterruptInternal(Robot& robot, double currentTime_sec, bool isShortInterrupt) override;

private:
  
  bool    _interrupted = false;
  u32     _actionRunning = 0;
  u8      _originalVisionModes = 0;
  bool    _initialReactionAnimPlayed = false;
  double  _lastInterruptTime_sec = std::numeric_limits<double>::min();
  
  // TODO: Read these from json config
  f32     _moveForwardDist_mm = 15.f;
  f32     _moveForwardSpeedIncrease = 2.f;
  Radians _driveForwardTol = DEG_TO_RAD(3.f); // both pan/tilt less than this will result in drive forward
  Radians _panAndTiltTol = DEG_TO_RAD(3.f);  // pan/tilt must be greater than this to actually turn
  double  _initialReactionWaitTime_sec = 20.f;
  
  virtual void HandleWhileRunning(const EngineToGameEvent& event, Robot& robot) override;

}; // class BehaviorFollowMotion
  

} // namespace Cozmo
} // namespace Anki

#endif // __Cozmo_Basestation_Behaviors_BehaviorFollowMotion_H__

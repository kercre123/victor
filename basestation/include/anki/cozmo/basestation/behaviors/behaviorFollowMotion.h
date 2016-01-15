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
  
  void StartTracking(Robot& robot);
  
  enum class State : u8 {
    BackingUp,
    WaitingForFirstMotion,      
    Tracking,
    HoldingHeadDown,
    BringingHeadUp,
    DrivingForward,
    Interrupted
  };
  
  // Internal state of the behavior:
  State       _state = State::WaitingForFirstMotion;
  u32         _actionRunning = (u32)ActionConstants::INVALID_TAG;
  u32         _backingUpAction = (u32)ActionConstants::INVALID_TAG;
  u8          _originalVisionModes = 0;
  bool        _initialReactionAnimPlayed = false;
  double      _lastInterruptTime_sec = std::numeric_limits<double>::lowest(); // Not min(), which is +ve!
  f32         _holdHeadDownUntil = -1.0f;
  
  // Configuration parameters:
  // TODO: Read these from json config
  f32     _moveForwardDist_mm = 15.f;
  f32     _moveForwardSpeedIncrease = 2.f;
  Radians _driveForwardTol = DEG_TO_RAD(5.f); // both pan/tilt less than this will result in drive forward
  f32     _minDriveFrowardGroundPlaneDist_mm = 120.0f;
  f32     _minGroundAreaToConsider = 0.05f;
  Radians _panAndTiltTol = DEG_TO_RAD(3.f);  // pan/tilt must be greater than this to actually turn
  double  _initialReactionWaitTime_sec = 20.f;
  f32     _timeToHoldHeadDown_sec = 1.25f;  // increment hold by this amount if motion observed
  
  // tracks how far we've driven forward in this behavior
  f32 _totalDriveForwardDist = 0.0f;
  f32 _additionalBackupDist = 20.0f;
  f32 _maxBackupDistance = 250.0f;
  f32 _backupSpeed = 80.0f;
  
  
  bool _lockedLift = false;
  void LiftShouldBeLocked(Robot& robot);
  void LiftShouldBeUnlocked(Robot& robot);
  
  virtual void HandleWhileRunning(const EngineToGameEvent& event, Robot& robot) override;
  void HandleObservedMotion(const EngineToGameEvent& event, Robot& robot);
  void HandleCompletedAction(const EngineToGameEvent& event, Robot& robot);
  
}; // class BehaviorFollowMotion
  

} // namespace Cozmo
} // namespace Anki

#endif // __Cozmo_Basestation_Behaviors_BehaviorFollowMotion_H__
